/*
$Header: /root/Signalogic/DirectCore/lib/pktlib/pktlib_RFC791_fragmentation.cpp
 
Description

  APIs for packet fragmentation and duplication (the latter is included as it's required for reliable reassembly)

Notes

  -fully multithreaded, no locks, no semaphore
  -no dependencies on other pktlib APIs
 
Projects

  SigSRF, DirectCore
 
Copyright Signalogic Inc. 2024

License

  Github SigSRF License, Version 1.1, https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

Documentation and Usage

  1) All fragmentation related API definitions and flags are documented on Github (https://github.com/signalogic/SigSRF_SDK/blob/master/pktlib_readme.md) and in pktlib.h
 
  2) Functions here are called by DSGetPacketInfo(), a pktlib API. pktlib is a SigSRF shared object library linked by mediaMin and mediaTest reference apps and user apps

  3) If you modify PktXxx() or DSXxx() functions, then place the resulting .o before libpktlib.so in your app link order, then your mods will take precedence over pktlib function names

Revision History

  Created Aug 2024, JHB, separated and moved here from pktlib source
  Modified Jun 2024 JHB, full fragmentation and reassembly implementation:
                         -each linked list fragment entry includes a PKT_FRAGMENT struct containing
                           (i) 3-way tuple info (IP src addr, dst addr), IP header identifier (Identification field), and fragment offset
                           (ii) packet info including flags, identifier, fragment offset, and saved IP header and packet data. See PKT_FRAGMENT struct in pktlib.h
                        -App_Thread_Info[] contains per app thread fragment linked list head; GetThreadIndex() uses a simple mem barrier lock to coordinate multithread access
                        -DS_PKT_INFO_FRAGMENT_xxx and DS_PKT_INFO_REASSEMBLY_xxx uFlags, and also DS_PKT_INFO_RETURN_xxx return flags, added to DSGetPacketInfo() definitions in pktlib.h 
                        -PktXxx() functions are internal APIs, expected to be called by DSGetPacketInfo() in pktlib
                        -To-do: IPv6
   Modified Aug 2024 JHB, fix g++ warnings
*/

/* Linux and/or other OS includes */

#include <stdio.h>

using namespace std;

/* SigSRF includes */

#include "pktlib.h"   /* pktlib API definitions, includes arpa/inet.h, netinet/if_ether.h */
#include "diaglib.h"  /* packet logging and diagnostics, including Log_RT() definition */

/* internal fragmentation functions and stats. Notes, JHB Jun 2024:

   -fragments are managed as per app thread linked lists; a simple mem barrier lock coordinates critical section thread access and modification of App_Thread_Info[]. The method works as long as the caller has a unique thread Id; for example, p/m threads could also call DSGetPacketInfo() with fragmented packets
   -each linked list fragment entry includes 3-way tuple info (protocol, IP src addr, IP dst addr), IP header identifier (Identification field), and fragment offset. See PKT_FRAGMENT struct in pktlib.h
   -each linked list entry also includes packet info: flags, identifier, fragment offset, and saved IP header and packet data
   -performance wise, the worst case is an app thread with a high number of streams each with large packets of size 4500 to 6000 bytes, in which case the thread's linked list length could grow to around N*3 or N*4, where N is number of streams
   -theoretically performance could be improved by adding a per stream linked list (as a sub list under the thread list) based on the 3-way tuple, but that involves creating a unique key or hash for each tuple. That step is time-consuming, involving memcmp's of 30+ bytes, so potential performance gain is unclear at the expense of increase in coding complexity and maintenance resources **
   -we maintain a "highest used location" (max_search_limit) to reduce search time for existing thread Ids. The search loop isn't doing much, just comparing 64-bit thread Id values
   
** I actually tried this in pktlib_new_frag_list.c ... I didn't get it completely working, some debug needed. But after seeing the amount of critical section code for the sub linked list, I put the effort on hold
*/

#ifdef __cplusplus
extern "C" {  /* make functions accessible to other pktlib C/C++ sources */
#endif

int PktAddFragment(uint8_t* pkt, unsigned int uFlags);
int PktFindFragment(uint8_t* pkt, unsigned int uFlags);
int PktGetReassemblyStatus(uint8_t* pkt, unsigned int uFlags);
int PktReassemble(uint8_t* pkt, unsigned int uFlags);

#ifdef __cplusplus
}
#endif

//#define FRAGMENTATION_DEBUG

#define MAX_APP_THREADS 128

typedef struct {

   pthread_t      ThreadId;               /* unique thread Id */
   PKT_FRAGMENT*  pPktFragmentList;       /* per thread packet fragment list */
   int            total_fragment_count;   /* total fragments handled by the app thread */
   int            active_fragment_count;  /* fragments currently active at any one time. DSPktRemoveFragment() can be called by an app thread during cleanup to get number of "orphan" fragments remaining on the thread's linked list */
   int            max_fragment_count;     /* max active fragments on the thread's link list */

} APP_THREAD_INFO;

static APP_THREAD_INFO App_Thread_Info[MAX_APP_THREADS] = {{ 0 }};
static int max_search_limit = 0;

static uint8_t app_thread_lock = 0;  /* mem barrier lock used for app thread synchronization */

/* local static APIs */

static int GetThreadIndex(bool fDelete) {  /* get thread Id index; create if not existing yet */

int i;

/* start critical section */

   while (__sync_lock_test_and_set(&app_thread_lock, 1) != 0);  /* set a memory barrier to coordinate concurrent app thread access to App_Thread_Info[]. Wait until the lock is zero then write 1 to it. While waiting keep writing 1 */

   for (i=0; i<max_search_limit; i++) if (App_Thread_Info[i].ThreadId == pthread_self()) {  /* look for existing thread id */

      if (fDelete) App_Thread_Info[i].ThreadId = 0;  /* delete if requested */
      goto exit;
   }

   if (i == max_search_limit) {  /* create if not found */

      for (i=0; i<MAX_APP_THREADS; i++) if (App_Thread_Info[i].ThreadId == 0) {  /* find first available index */

         App_Thread_Info[i].ThreadId = pthread_self();  /* initialize the new index */
         max_search_limit = max(max_search_limit, i+1);  /* adjust max search limit */
         #if 0
         printf("\n *** creating linked list node, max_search_limit = %d \n", max_search_limit);
         #endif
         goto exit;
      }
   }

   if (i >= MAX_APP_THREADS) i = -1;  /* return error condition - too many app threads */

exit:
/* end critical section */
   __sync_lock_release(&app_thread_lock);  /* clear the mem barrier (write 0 to the lock) */
   return i;  /* return thread Id index */
}

__attribute__ ((unused)) static int DeleteThreadIndex(void) {  /* delete thread index (currently unused) */

   return GetThreadIndex(true);
}

/* inline helper functions for PktXxxFragment() functions */

static inline void get_3way_tuple(uint8_t* pkt, uint8_t* protocol, unsigned __int128* ip_src_addr, unsigned __int128* ip_dst_addr, unsigned int uFlags) {

  if (pkt) {

      if (protocol) *protocol = pkt[9];

      uint8_t version = pkt[0] >> 4;

      if (version == IPv4) {

         if (uFlags & DS_PKTLIB_HOST_BYTE_ORDER) memcpy(ip_src_addr, &pkt[12], 4);  /* copy src IP addr */
         else { uint32_t* p32 = (uint32_t*)ip_src_addr; *p32 = ((uint32_t)pkt[12] << 24) | ((uint32_t)pkt[13] << 16) | ((uint32_t)pkt[14] << 8) | (uint32_t)pkt[15]; }

         if (uFlags & DS_PKTLIB_HOST_BYTE_ORDER) memcpy(ip_dst_addr, &pkt[16], 4);  /* copy dst IP addr */
         else { uint32_t* p32 = (uint32_t*)ip_dst_addr; *p32 = ((uint32_t)pkt[16] << 24) | ((uint32_t)pkt[17] << 16) | ((uint32_t)pkt[18] << 8) | (uint32_t)pkt[19]; }
      }
      else if (version == IPv6) {  /* IPv6 addresses are always in host byte order (https://www.gnu.org/software/guile/manual/html_node/Network-Address-Conversion.html) */
 
         memcpy(ip_src_addr, &pkt[8], 16);  /* copy src IP addr */
         memcpy(ip_dst_addr, &pkt[24], 16);  /* copy dst IP addr */
      }
   }
}

static inline void get_identifier_and_offset(uint8_t* pkt, uint16_t* identifier, uint16_t* fragment_offset, unsigned int uFlags) {

  if (pkt && identifier) *identifier = (uFlags & DS_PKTLIB_HOST_BYTE_ORDER) ? (pkt[5] << 8) | pkt[4] : (pkt[4] << 8) | pkt[5];

  if (pkt && fragment_offset) *fragment_offset = (uFlags & DS_PKTLIB_HOST_BYTE_ORDER) ? ((pkt[7] & 0x1f) << 8) | pkt[6] : ((pkt[6] & 0x1f) << 8) | pkt[7];
}

/* private fragment management APIs */

/* add packet fragment to app thread's fragment list */

int PktAddFragment(uint8_t* pkt, unsigned int uFlags) {

   if (!pkt) return -1;  /* error condition */

/* allocate linked list fragment struct mem */

   PKT_FRAGMENT* pPktFrag = (PKT_FRAGMENT*)calloc(1, sizeof(PKT_FRAGMENT));  /* create new fragment list item, initialize all items to zero (especially fragment list head and IP src/dst addrs) */

   if (!pPktFrag) return -1;  /* error condition */

/* populate fields of new fragment struct */

/* protocol + IP src addr + IP dst addr form a 3-way tuple used to uniquely identify stream / connection between endpoints. This prevents potential confusion of Identifiers (16-bit Identification field) between streams, especially after long durations where 16-bit Ids may wrap. Mentioned in RFCs 6864 and 6146 */

   get_3way_tuple(pkt, &pPktFrag->protocol, &pPktFrag->ip_src_addr, &pPktFrag->ip_dst_addr, uFlags);
   get_identifier_and_offset(pkt, &pPktFrag->identifier, &pPktFrag->offset, uFlags);

   pPktFrag->flags = ((pkt[(uFlags & DS_PKTLIB_HOST_BYTE_ORDER) ? 7 : 6] >> 5) & 1) ? DS_PKT_FRAGMENT_MF : 0;
   if (pPktFrag->offset) pPktFrag->flags |= DS_PKT_FRAGMENT_OFS;

/* save packet header and payload */

   int ip_hdr_len = DSGetPacketInfo(-1, (uFlags & DS_PKTLIB_HOST_BYTE_ORDER) | DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_HDRLEN, pkt, -1, NULL, NULL);  /* recursive calls but not a problem uFlags does not include fragment or PKTINFO related flags */
   int pkt_len = DSGetPacketInfo(-1, (uFlags & DS_PKTLIB_HOST_BYTE_ORDER) | DS_BUFFER_PKT_IP_PACKET | DS_PKT_INFO_PKTLEN, pkt, -1, NULL, NULL);

   if (ip_hdr_len <= 0 || pkt_len <= 0) return -1;

   if (!(pPktFrag->pkt_buf = (uint8_t*)malloc(pkt_len))) return -1;
   if (!(pPktFrag->ip_hdr_buf = (uint8_t*)malloc(ip_hdr_len))) return -1;

/* save IP header info in fragment list entry. Technically only the first fragment (with offset 0) needs to be copied but we can receive fragments out-of-order, so we give PktReassemble() all info it might need at time of reassembly */

   pPktFrag->ip_hdr_len = ip_hdr_len;
   memcpy(pPktFrag->ip_hdr_buf, pkt, pPktFrag->ip_hdr_len);

/* save packet data in fragment list entry */

   pPktFrag->len = pkt_len - ip_hdr_len;
   memcpy(pPktFrag->pkt_buf, &pkt[ip_hdr_len], pPktFrag->len);

/* get App_Thread_Info[] index for current thread. If not existing GetThreadIndex() will create a new one */

   int thread_index = GetThreadIndex(false), list_count = 1;

   #ifdef FRAGMENTATION_DEBUG
   printf("\n *** inside pkt_frag_add, active fragments = %d, flags = 0x%x, identifier = %d, offset = %d, pkt len = %d \n", App_Thread_Info[thread_index].active_fragment_count, pPktFrag->flags, pPktFrag->identifier, pPktFrag->offset, pPktFrag->len);
   #endif

   if (!App_Thread_Info[thread_index].pPktFragmentList) App_Thread_Info[thread_index].pPktFragmentList = pPktFrag;  /* empty list: add as first fragment */
   else {

      PKT_FRAGMENT* pList = App_Thread_Info[thread_index].pPktFragmentList;

      while (pList->next) { pList = pList->next; list_count++; }  /* find end of list, count number of list nodes */

      pList->next = pPktFrag;  /* add to end of list */
   }

   App_Thread_Info[thread_index].active_fragment_count++;  /* increment fragment counts */
   App_Thread_Info[thread_index].total_fragment_count++;

   if (list_count > App_Thread_Info[thread_index].max_fragment_count) App_Thread_Info[thread_index].max_fragment_count = list_count;  /* update max_fragment_count */

   return DS_PKT_INFO_RETURN_FRAGMENT | DS_PKT_INFO_RETURN_FRAGMENT_SAVED;  /* To-do: error checking, is there anything that can go wrong here */
}

/* walk app thread's fragment list and look for existing fragment, uniquely identified by Identification field and fragment offset */

int PktFindFragment(uint8_t* pkt, unsigned int uFlags) {

PKT_FRAGMENT* pList = App_Thread_Info[GetThreadIndex(false)].pPktFragmentList;

uint8_t protocol = 0;  /* don't need to be initialized, only to avoid compiler warnings */
unsigned __int128 ip_src_addr = 0, ip_dst_addr = 0;  /* be sure to init IP addrs to zero as IPv4 uses only 4 out of 16 bytes */
uint16_t identifier = 0, fragment_offset = 0;

   get_3way_tuple(pkt, &protocol, &ip_src_addr, &ip_dst_addr, uFlags);

   get_identifier_and_offset(pkt, &identifier, &fragment_offset, uFlags);

   while (pList) {

      if (protocol == pList->protocol && ip_src_addr == pList->ip_src_addr && ip_dst_addr == pList->ip_dst_addr && identifier == pList->identifier && fragment_offset == pList->offset) return DS_PKT_INFO_RETURN_FRAGMENT;  /* fragment found if 3-way tuple, identifier, and offset all match */

      pList = pList->next;
   }

   return 0;  /* not found */
}

/* remove a fragment from app thread's fragment list. If pkt is NULL, remove all fragments */

int DSPktRemoveFragment(uint8_t* pkt, unsigned int uFlags, unsigned int* max_list_fragments) {

int thread_index = GetThreadIndex(false), nRemoved = 0;
PKT_FRAGMENT* pList = App_Thread_Info[thread_index].pPktFragmentList, *pListPrev = NULL, *pListNext = NULL;

uint8_t protocol = 0;  /* don't need to be initialized, only to avoid compiler warnings */
unsigned __int128 ip_src_addr = 0, ip_dst_addr = 0;
uint16_t identifier = 0, fragment_offset = 0;

   get_3way_tuple(pkt, &protocol, &ip_src_addr, &ip_dst_addr, uFlags);

   get_identifier_and_offset(pkt, &identifier, &fragment_offset, uFlags);

/* remove fragment(s) with matching identifier and offset, free allocated mem */

   while (pList) {

      pListNext = pList->next;  /* save next ptr in case pList is freed */

      if (!pkt || (protocol == pList->protocol && ip_src_addr == pList->ip_src_addr && ip_dst_addr == pList->ip_dst_addr && identifier == pList->identifier && fragment_offset == pList->offset)) {  /* 3-way tuple, identifier, and offset all have to match. Note pkt can be NULL, in which case we remove all remaining nodes (cleanup) */

         if (pListPrev) pListPrev->next = pList->next;  /* remove fragment from the list and update last non-matching fragment to point to next fragment */ 
         else App_Thread_Info[thread_index].pPktFragmentList = pList->next;  /* if fragment was at start of the list then move the list head */

         free(pList->ip_hdr_buf);  /* free IP header buffer */
         free(pList->pkt_buf);  /* free packet data buffer */
         free(pList);  /* free fragment list entry */

         nRemoved++;

         App_Thread_Info[thread_index].active_fragment_count--;  /* decrement fragment count */
      }
      else pListPrev = pList;  /* update last non-matching fragment */

      pList = pListNext;
   }

   #ifdef FRAGMENTATION_DEBUG
   printf("\n *** inside pkt removed %d fragments, active fragments = %d, identifier = %d, offset = %d \n", nRemoved, App_Thread_Info[thread_index].active_fragment_count, identifier, fragment_offset);
   #endif

   if (max_list_fragments) *max_list_fragments = App_Thread_Info[thread_index].max_fragment_count;  /* return max list fragments stat if requested */

   return pkt ? (nRemoved ? DS_PKT_INFO_RETURN_FRAGMENT_REMOVED : 0) : nRemoved;  /* if pkt NULL return number removed, otherwise return status flag */ 
}

/* check if all fragments are available for reassembly. Note this is independent of packet receive order */

int PktGetReassemblyStatus(uint8_t* pkt, unsigned int uFlags) {

int thread_index = GetThreadIndex(false), ret_val = 0;
PKT_FRAGMENT* pList = App_Thread_Info[thread_index].pPktFragmentList;
uint16_t reassembled_len = 0;
#if 0
uint16_t matching_fragments = 0;
#endif

uint8_t protocol = 0;  /* don't need to be initialized, only to avoid compiler warnings */
unsigned __int128 ip_src_addr = 0, ip_dst_addr = 0;
uint16_t identifier = 0;

   get_3way_tuple(pkt, &protocol, &ip_src_addr, &ip_dst_addr, uFlags);

   get_identifier_and_offset(pkt, &identifier, NULL, uFlags);

/* first sum lengths of currently available fragments */

   while (pList) {

      if (protocol == pList->protocol && ip_src_addr == pList->ip_src_addr && ip_dst_addr == pList->ip_dst_addr && identifier == pList->identifier) {  /* 3-way tuple and identifier have to match, but not offsets, which can be in any order */

         reassembled_len += pList->len;
         ret_val |= DS_PKT_INFO_RETURN_FRAGMENT;
         #if 0
         matching_fragments++;
         #endif
      }

      pList = pList->next;
   }

/* second check if all fragments have arrived and math of total lengths vs final offset checks out */

   pList = App_Thread_Info[thread_index].pPktFragmentList;

   while (pList) {  /* if last fragment has arrived, and its offset matches sum of lengths received, then we have all fragments */

      if (protocol == pList->protocol && ip_src_addr == pList->ip_src_addr && ip_dst_addr == pList->ip_dst_addr && identifier == pList->identifier) {  /* 3-way tuple and identifier have to match */

         if (!(pList->flags & DS_PKT_FRAGMENT_MF)) {

//  printf("\n *** last fragment, identifier = %d, 8x offset + len = %d, total len = %d \n", pList->identifier, pList->offset*8 + pList->len, total_length);

            if (pList->offset*8 + pList->len == reassembled_len) { ret_val |= DS_PKT_INFO_RETURN_REASSEMBLED_PACKET_AVAILABLE; break; }  /* all fragments received */
         }
      }

      pList = pList->next;
   }

   return ret_val;
}

/* walk app thread's fragment list to find matching identifiers, copy IP header and reassembled packet data, remove fragments from list, return total packet length */
  
int PktReassemble(uint8_t* pkt, unsigned int uFlags) {

int thread_index = GetThreadIndex(false), matching_fragments = 0;
PKT_FRAGMENT* pList = App_Thread_Info[thread_index].pPktFragmentList, *pListPrev = NULL, *pListNext = NULL;
uint16_t reassembled_len = 0, ip_hdr_len = 0;

uint8_t protocol = 0;  /* don't need to be initialized, only to avoid compiler warnings */
unsigned __int128 ip_src_addr = 0, ip_dst_addr = 0;
uint16_t identifier = 0;

   get_3way_tuple(pkt, &protocol, &ip_src_addr, &ip_dst_addr, uFlags);

   get_identifier_and_offset(pkt, &identifier, NULL, uFlags);

/* reassemble full packet from matching fragment saved data, free allocated mem */

   while (pList) {  /* first locate fragment 0 IP header len, in case fragments are received out of order and have different IP header lens */

      if (protocol == pList->protocol && ip_src_addr == pList->ip_src_addr && ip_dst_addr == pList->ip_dst_addr && identifier == pList->identifier) {  /* 3-way tuple and identifier have to match */

         if (pList->offset == 0) { ip_hdr_len = pList->ip_hdr_len; break; }
      }

      pList = pList->next;
   }

   pList = App_Thread_Info[thread_index].pPktFragmentList;

   if (ip_hdr_len) while (pList) {  /* then run the loop again and reassemble packet data */

      pListNext = pList->next;  /* save next ptr in case pList is freed */

      if (protocol == pList->protocol && ip_src_addr == pList->ip_src_addr && ip_dst_addr == pList->ip_dst_addr && identifier == pList->identifier) {  /* 3-way tuple and identifier have to match */

         matching_fragments++;

         reassembled_len += pList->len;

         if (pList->offset == 0) memcpy(pkt, pList->ip_hdr_buf, ip_hdr_len);  /* copy IP header from first fragment */

         memcpy(&pkt[pList->ip_hdr_len + pList->offset*8], pList->pkt_buf, pList->len);  /* copy packet data into reassembly position given by fragment offset */

         if (pListPrev) pListPrev->next = pList->next;  /* remove fragment from the list and update last non-matching fragment to point to next fragment */ 
         else App_Thread_Info[thread_index].pPktFragmentList = pList->next;  /* if fragment was at start of the list then move the list head */

         free(pList->ip_hdr_buf);  /* free IP header buffer */
         free(pList->pkt_buf);  /* free packet data buffer */
         free(pList);  /* free fragment list entry */
      }
      else pListPrev = pList;  /* update last non-matching fragment */

      pList = pListNext;
   }

   int pkt_len = ip_hdr_len + reassembled_len;

   if (pkt_len) {  /* adjust reassembled packet header */

      pkt[(uFlags & DS_PKTLIB_HOST_BYTE_ORDER) ? 7 : 6] &= 0xc0;  /* remove original packet fragmentation info */
      pkt[(uFlags & DS_PKTLIB_HOST_BYTE_ORDER) ? 6 : 7] = 0;
      pkt[2] = (uFlags & DS_PKTLIB_HOST_BYTE_ORDER) ? pkt_len & 0xff : pkt_len >> 8;  /* update length */
      pkt[3] = (uFlags & DS_PKTLIB_HOST_BYTE_ORDER) ? pkt_len >> 8 : pkt_len & 0xff;

      App_Thread_Info[thread_index].active_fragment_count -= matching_fragments;  /* reduce active count by number of reassembly fragments */

      #ifdef FRAGMENTATION_DEBUG
      printf("\n *** reassembled packet returned, identifier = %d, total fragments = %d, active fragments = %d, pkt len = %d, pFragmentList = %p \n", identifier, App_Thread_Info[thread_index].total_fragment_count, App_Thread_Info[thread_index].active_fragment_count, pkt_len, App_Thread_Info[thread_index].pPktFragmentList);
      #endif
   }

   return pkt_len;  /* return reassembled packet length */
}


int DSIsReservedUDP(uint16_t port) {

   switch (port) {

      case 137:
      case 138:   /* NetBIOS */
         return true;

      case 9009:  /* pichat */
         return true;

      case 547:   /* DHCPv6 */
         return true;

      case GTP_PORT:
         return true;
   }
   
   return false;
}

/* DSIsPacketDuplicate() compares packets for exact copies:

   -PktInfo1 should point to PKTINFO struct from current packet
   -PktInfo2 should point to PKTINFO struct from earlier packet
   -pInfo is optional param, can be used in debug printout or other purposes based on DS_PKT_DUPLICATE_XXX uFlags options. Not used if zero
   -returns true or false as an int; this may change if further return values are needed

   -UDP duplicates are substantially more complex to detect; see comments
*/

int DSIsPacketDuplicate(unsigned int uFlags, PKTINFO* PktInfo1, PKTINFO* PktInfo2, void* pInfo) {

   if (PktInfo1->protocol != PktInfo2->protocol) return false;  /* immediate return if protocols not the same */
   
   if (PktInfo1->protocol == TCP) {

      /* remove redundant TCP retransmissions. Notes, JHB Apr 2023:

         -streams may contain redundant TCP retransmission of some or all packets. Normally this may be due to transmission errors, but appears there are other cases also, such as for FEC purposes like F5 does, or some HI2/HI3 streams where every packet is duplicated
         -we detect and strip these out. Sequence numbers, length, and ports must be an exact copy
         -currently this is a rudimentary implementation, not likely to work with multiple/mixed TCP sessions
         -to-do: implement TCP session management, separate but similar to existing UDP sessions handled by pktlib
      */

      if (PktInfo1->seqnum == PktInfo2->seqnum && PktInfo1->ack_seqnum == PktInfo2->ack_seqnum && PktInfo1->pkt_len == PktInfo2->pkt_len && PktInfo1->dst_port == PktInfo2->dst_port && PktInfo1->src_port == PktInfo2->src_port) {

         return true;
      }
   }
   else if (PktInfo1->protocol == UDP) {

   /* UDP/RTP packets are not typically duplicated with exception of RFC 7198, which applies to RTP media and is handled in pktlib. However, in general (not RTP) fragmented UDP packets (for example long SIP messages and SDP info descriptions) and certain ports may be duplicated because senders are worried about dropping the packet, making reassembly impossible or losing key network control info (e.g. DHCP). PushPackets() in mediaMin.cpp calls DSIsPacketDuplicate() to look for such UDP packets and if found strips them out. Notes, JHB Jun 2024:

      -UDP checksums are ignored -- unreliable due to Wireshark warning about "UDP checksum offload". There is a lot of online discussion about this

      -certain packets sent to certain ports are looked at, including GTP, DHCP, and NetBIOS. This likely needs refinement for RTP over GTP, in which case we need to let same-SSRC detection and RFC 7198 make duplication decisions

      -UDP duplicates appearing 2 or more packets later are not currently detected. This may be the case if you see a console message like:

        ignoring UDP SIP fragment packet (2), pkt len = 653, frag flags = 0x2, last keyword search = "application"

      -pktlib's RFC 7198 implementation will "look back" up to 8 packets. mediaMin allows control over this with the -lN command line option where N is the number of lookback packets
   */

      if ((uFlags & DS_PKT_DUPLICATE_PRINT_PKTNUMBER) && pInfo) {  /* debug print out if specified in uFlags */

         char szPktNumber[20] = "";
         if (pInfo) sprintf(szPktNumber, "%d", *((int*)pInfo));
   
         char tmpstr[400];
         sprintf(tmpstr, "\n *** inside DSIsPacketDuplicate() pkt number %s, len = %d, len prev = %d, flags = 0x%x flags prev = 0x%x, offset = %d offset prev = %d, ip hdr checksum = 0x%x, ip hdr checksum prev = 0x%x", szPktNumber, PktInfo1->pkt_len, PktInfo2->pkt_len, PktInfo1->flags, PktInfo2->flags, PktInfo1->fragment_offset, PktInfo2->fragment_offset, PktInfo1->ip_hdr_checksum, PktInfo2->ip_hdr_checksum);
         if (PktInfo1->fragment_offset == 0) sprintf(&tmpstr[strlen(tmpstr)], " udp checksum = 0x%x, udp checksum prev = 0x%x", PktInfo1->udp_checksum, PktInfo2->udp_checksum);
         printf("%s \n", tmpstr);
      }

      bool fFragmentCompare = (PktInfo1->flags & DS_PKT_FRAGMENT_ITEM_MASK) && (PktInfo1->flags & DS_PKT_FRAGMENT_ITEM_MASK) == (PktInfo2->flags & DS_PKT_FRAGMENT_ITEM_MASK);  /* both current and previous packet contain identical non-zero fragment flags ? */
      bool fPortCompare = DSIsReservedUDP(PktInfo1->dst_port) && PktInfo1->dst_port == PktInfo2->dst_port;  /* both current and previous packet are sent to specific dst ports ? Test with codecs-amr-12.pcap, codecs3-amr-wb.pcap, VIDEOCALL_EVS_H265.pcapng, JHB Jul 2024 */

      if (
          (fFragmentCompare || fPortCompare) &&
          (
           (!(uFlags & DS_PKT_DUPLICATE_INCLUDE_UDP_CHECKSUM) || PktInfo1->udp_checksum == PktInfo2->udp_checksum) &&  /* ignore UDP checksum unless specified in uFlags. Ignoring is the default, as noted above comments. Maybe there is some way to know when / when not */
           (PktInfo1->fragment_offset != 0 || PktInfo2->fragment_offset != 0 || PktInfo1->pyld_len == PktInfo2->pyld_len) &&  /* compare UDP payload lengths if both fragment offsets are zero */
           PktInfo1->pkt_len == PktInfo2->pkt_len &&  /* compare packet lengths */
           PktInfo1->ip_hdr_checksum == PktInfo2->ip_hdr_checksum  /* compare IP header checksums, which implicitly compares IP header lengths */
          )
         ) {

         return true;
      }
   }

   return false;
}
