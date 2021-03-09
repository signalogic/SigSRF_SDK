/*
 $Header: /root/Signalogic/DirectCore/include/derlib.h
 
 Copyright (C) Signalogic Inc. 2021

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description

  DER decoding library API header file to support ETSI LI HI2 and HI3 DER encoded data per ASN.1 format

 Projects

   SigSRF, DirectCore
 
 Revision History
  
  Created Feb 2021 JHB
*/
 
#ifndef _DERLIB_H_
#define _DERLIB_H_

#ifdef _DERLIB_INSTALLED_  /* _DERLIB_INSTALLED defined in Makefile if libwandder install found on the system */
  #include "der.h"  /* use libwandder header file if found during make */
#endif

#define MAX_DER_STREAMS                 256  /* max number of concurrent DER streams */

#define MAX_DER_STRLEN                  512  /* max length of DER encoded strings that derlib can handle */

/*defines used by DSconfigDerlib() API */

#define DS_CD_GLOBALCONFIG              0x01
#define DS_CD_DEBUGCONFIG               0x02
#define DS_CD_INIT                      0x04

/* defines used by DSDetectDerStream() */

#define DS_ISDER_INTERCEPTPOINTID       1       /* get DER stream interception point ID */
#define DS_ISDER_DSTPORT                2       /* get DER stream destination port */
#define DS_ISDER_PORT_MUST_BE_EVEN      0x1000  /* specify intercept data has to be received on even port number */

/* defines used by DSGetDerStreamInfo() */

#define DS_DER_INFO_DSTPORT            0x100
#define DS_DER_INFO_INTERCEPTPOINTID   0x200
#define DS_DER_INFO_ASN_INDEX          0x300
#define DS_DER_INFO_CC_PKT_COUNT       0x400

#define DS_DER_INFO_ITEM_MASK          0xff00

/* defines used by DSDecodeDerStream() */

#define DS_DER_NULL_PACKET              1
#define DS_DER_KEEP_ALIVE               2
#define DS_DER_SEQNUM                   4
#define DS_DER_INTERCEPTPOINTID         8
#define DS_DER_TIMESTAMP                0x10
#define DS_DER_TIMESTAMPQUALIFIER       0x20
#define DS_DER_CC_PACKET                0x40

#define DS_DECODE_DER_PRINT_DEBUG_INFO  0x10000000L

/* ASN.1 tag definitions */

#ifdef _DERLIB_INSTALLED_  /* use OpenLI tag definitions if available */

  #define DER_TAG_CLASS_APPLICATION_PRIMITIVE  WANDDER_CLASS_APPLICATION_PRIMITIVE
  #define DER_TAG_OID                          WANDDER_TAG_OID
  #define DER_TAG_OCTETSTRING                  WANDDER_TAG_OCTETSTRING
  #define DER_TAG_OBJECTDESCRIPTOR             WANDDER_TAG_OBJDESC
#else
  #define DER_TAG_CLASS_APPLICATION_PRIMITIVE  2
  #define DER_TAG_OID                          6
  #define DER_TAG_OCTETSTRING                  4
  #define DER_TAG_OBJECTDESCRIPTOR             7
#endif

#define DER_TAG_EXTERNAL                       8
#define DER_TAG_CLASS_CONSTRUCT                1

#define DER_TAG_INTERCEPTPOINTID               ((DER_TAG_CLASS_APPLICATION_PRIMITIVE << 6) | DER_TAG_OID)
#define DER_TAG_SEQNUM                         ((DER_TAG_CLASS_APPLICATION_PRIMITIVE << 6) | DER_TAG_OCTETSTRING)
#define DER_TAG_TIMESTAMP                      ((DER_TAG_CLASS_APPLICATION_PRIMITIVE << 6) | (DER_TAG_CLASS_CONSTRUCT << 5) | DER_TAG_OBJECTDESCRIPTOR)
#define DER_TAG_TIMESTAMPQUALIFIER             ((DER_TAG_CLASS_APPLICATION_PRIMITIVE << 6) | DER_TAG_EXTERNAL)

#ifdef __cplusplus
extern "C" {
#endif

  typedef int HDERSTREAM;  /* DER stream handle */

  typedef struct {
     uint16_t  tag;
     uint16_t  len;
     uint64_t  value;
     char      str[MAX_DER_STRLEN];
  } DER_ITEM;

  typedef struct {

    uint64_t  uList;           /* DSDecodeDerStream() sets a list of valid DS_DER_XX items found */
    int       asn_index;       /* for long/aggregated packets with multiple DER items, this is the current ASN.1 decoding offset (from start of packet payload) after each call to DSDecodeDerStream() */

    DER_ITEM  sequenceNumber;  /* intercept sequence number */
    DER_ITEM  interceptionPointId;
    DER_ITEM  timeStamp;
    DER_ITEM  timeStamp_sec;   /* timestamp seconds since 1 Jan 1970 */
    DER_ITEM  timeStamp_usec;  /* timestamp usec */
    DER_ITEM  timeStampQualifier;
    DER_ITEM  cc_packet;       /* note that CC packet contents are stored in DSDecodeDerStream() output param pkt_out_buf, not in DER_ITEM value or str[] items. Tag and len are stored */

  } HI3_DER_DECODE;

  /* DSConfigDerLib() initializes and configures derlib

     -has to be called once at app init time, by only one thread
     -uFlags options given by DS_CD_XX items above
     -return value < 0 indicates an error condition
  */

  int DSConfigDerlib(GLOBAL_CONFIG* pGlobalConfig, DEBUG_CONFIG* pDebugConfig, unsigned int uFlags);

  /* DSIsDerStream() finds and/or auto-detects a DER encoded stream

     -pkt_in_buf should contain a standard IPv4 or IPv6 TCP/IP packet, including header(s) and payload
     -uFlags options given by DS_ISDER_XX items above
     -szInterceptPointId must contain a valid string if DS_DER_GET_INTERCEPTPOINTID not specified
     -return value < 0 indicates an error condition
  */

  int DSIsDerStream(uint8_t* pkt_in_buf, unsigned int uFlags, char* szInterceptPointId, uint16_t* dest_port);

  /* DSCreateDerStream() creates a DER stream

     -returns a handle to a new DER stream
     -szInterceptPointId must contain a valid string, either user-supplied or determined by prior call to DSDetectDerStream()
     -return value < 0 indicates an error condition
  */

  HDERSTREAM DSCreateDerStream(const char* szInterceptPointId, uint16_t intercept_dest_port, unsigned int uFlags);

  /* DSGetDerStreamInfo() retrieves DER stream info

     -hDerStream must be a DER stream handle created by a prior call to DSCreateDerStream()
     -uFlags options given by DS_DER_INFO_XX items above
     -return value < 0 indicates an error condition
  */

  int64_t DSGetDerStreamInfo(HDERSTREAM hDerStream, unsigned int uFlags, void* pInfo);

  /* DSDecodeDerStream() decodes a DER encoded stream, returning one or more decoded items

     -hDerStream must be a DER stream handle created by a prior call to DSCreateDerStream()
     -pkt_in_buf should contain a standard IPv4 or IPv6 TCP/IP packet, including header(s) and payload
     -uFlags options given by DS_DER_XX items above
     -der_decode should point to a HI3_DER_DECODE struct to be filled in with decoded items
     -return value < 0 indicates an error condition
  */

  int DSDecodeDerStream(HDERSTREAM hDerStream, uint8_t* pkt_in_buf, uint8_t* pkt_out_buf, unsigned int uFlags, HI3_DER_DECODE* der_decode);

  /* DSDeleteDerStream() deletes a DER stream

     -hDerStream must be a DER stream handle created by a prior call to DSCreateDerStream()
     -return value < 0 indicates an error condition
  */

  int DSDeleteDerStream(HDERSTREAM hDerStream);

#ifdef __cplusplus
}
#endif

#endif
