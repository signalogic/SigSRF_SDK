/*
  ntplib.h

  NTP struct definitions and functions to support updating NTP values on coCPU hardware that is not implementing its own NTP protocol handling
  
  Notes
  
    1) Example use cases include coCPU functions for RTCP packet handling that need accurate wall clock times

    2) This is a shared header file between host and coCPU source code builds
  
  Copyright © Signalogic, 2015-2017

  Revision History:

    Created 2015, SC
    Modified Nov 2016 JHB, moved to shared_include subfolder
    Modified Aug 2017 CKJ, added alternative definition for alculateNTPTimeStamp() for ATCA builds
*/


/* c66x reserved areas in shared mem include:

  Usage       origin         length
  -----       --------       ------

  CoreSync    0x0c3fff00     32
  TSC_copy    0x0c3fff20     8
  Reserved    0x0c3fff28     216

  Notes;

  1) Above areas are reserved in the platform definition, but do not appear in the .map file
  
  2) Host code should do these steps:
  
       (a) read TSC_copy from coCPU mem
       (b) get wall clock time from Linux API (assumes Linux is running NTP daemon)
       (c) write TSC_copy and wall clock time to MostRecentNTPTimeStamp_struct in coCPU mem
     
  3) Time lag between above steps should be minimized to extent possible in order to allow coCPU to maintain accurate time
  
  4) MostRecentNTPTimeStamp_struct should be coCPU global var (should appear in the .map file)
  
  5) TSC = Timestamp counter, which is a coCPU "run-time life" value, i.e. time that coCPU has been
     running in CPU cycles (note, for c66x this is 64-bit TSC mem-mapped register)

*/

#ifndef _NTP_LIB_H_
#define _NTP_LIB_H_

#include <stdlib.h>

#define EPOCH 2208988800ULL
#define TSC_copy 0xC3FFF20  /* address of 8 byte TSC value, see notes above above */


/* shared mem struct usedby host and coCPU when updating and/or calculating NTP based time values */

typedef struct {

/* Both base values should be established as close to simultaneously as possible.  Writes for these values to coCPU mem should be done as atomically as possible */

   unsigned long long TSC;  /* value read from coCPU hardware, if zero then should not be used */
   unsigned long long NTP;  /* current time value from remote NTP server */

} MostRecentNTPTimeStamp_struct;


/* Host function to update coCPU with NTP values.  Notes:

  1) Platform dependent implementation in ntplib.so
  
  2) ATCA host transcoding builds are not using this API yet, instead they are using mailbox communication and sending two words in the mail message payload
  
  3) PCIe host transcoding builds (e.g. mediaTest) should implement this API
  
  4) streamTest.c has an UpdateTargetNTP() function that should implement this API but currently (Aug2017) does not yet
*/

unsigned long long UpdateNTPcoCPU(MostRecentNTPTimeStamp_struct*);

/* coCPU function to read NTP values from host and update local NTP time.  Notes:

  1) Platform dependent implementation in ntplib.c
  
  2) rtcp_sender_report.c uses this function
*/

#if defined(_USE_PCIE_) || defined (_USE_ATCA_) || defined(_ADV8901_)
  void calculateNTPTimeStamp(uint32_t *rtcpLsw, uint32_t *rtcpMsw);
#else
  unsigned long long calculateNTPTimeStamp(MostRecentNTPTimeStamp_struct*);
#endif

#endif // _NTP_LIB_H_
