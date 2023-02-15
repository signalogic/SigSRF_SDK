/*
 $Header: /root/Signalogic/DirectCore/include/derlib.h
 
 Copyright (C) Signalogic Inc. 2021

 License

  Use and distribution of this source code is subject to terms and conditions of the Github SigSRF License v1.0, published at https://github.com/signalogic/SigSRF_SDK/blob/master/LICENSE.md

 Description

  DER decoding library API header file to support ETSI LI HI2 and HI3 DER encoded data

 Projects

   SigSRF, DirectCore
 
 Revision History
  
  Created Feb 2021 JHB
  Modified May 2021 JHB, add DSSetDerStreamInfo()
  Modified May 2021 JHB, in DSFindDerStream() change dest_port* parameter to point to a list of ports (list is zero-terminated)
                         -add DS_DER_INFO_DSTPORT_LIST to DSGetDerStreamInfo()
                         -change operation of DS_DER_INFO_DSTPORT to get/set a specific port in DSGetDerStreamInfo() and DSSetDerStreamInfo()
  Modified Jun 2021 JHB, additional comments / instructions
  Modified Dec 2022 JHB, add tag definitions, DSDecodeDerFields() API, which includes XML output option (per ETSI LI ASN.1 specs)
*/
 
#ifndef _DERLIB_H_
#define _DERLIB_H_

#ifdef _DERLIB_INSTALLED_  /* _DERLIB_INSTALLED defined in Makefile if libwandder install found on the system */
  #include "der.h"  /* use libwandder header file if found during make */
#endif

#define MAX_DER_STREAMS                          256  /* max number of concurrent DER streams */
#define MAX_DER_DSTPORTS                         16

#define MAX_DER_STRLEN                           512  /* max length of DER encoded strings that derlib can handle */

/* uFlags for DSconfigDerlib() API */

#define DS_CD_GLOBALCONFIG                       1
#define DS_CD_DEBUGCONFIG                        2
#define DS_CD_INIT                               4

/* uFlags for DSFindDerStream() */

#define DS_DER_FIND_INTERCEPTPOINTID             1        /* find DER stream interception point ID */
#define DS_DER_FIND_DSTPORT                      2        /* find DER stream destination port(s) */
#define DS_DER_FIND_PORT_MUST_BE_EVEN            0x1000   /* specify intercept data has to be received on even port number */

/* uFlags for DSGetDerStreamInfo() and DSSetDerStreamInfo() */

#define DS_DER_INFO_DSTPORT                      0x100
#define DS_DER_INFO_INTERCEPTPOINTID             0x200
#define DS_DER_INFO_ASN_INDEX                    0x300
#define DS_DER_INFO_CC_PKT_COUNT                 0x400
#define DS_DER_INFO_DSTPORT_LIST                 0x500

#define DS_DER_INFO_ITEM_MASK                    0xff00

/* uFlags for DSDecodeDerStream() */

#define DS_DER_NULL_PACKET                       1
#define DS_DER_KEEP_ALIVE                        2
#define DS_DER_SEQNUM                            4
#define DS_DER_INTERCEPTPOINTID                  8
#define DS_DER_TIMESTAMP                         0x10
#define DS_DER_TIMESTAMPQUALIFIER                0x20
#define DS_DER_CC_PACKET                         0x40

#define DS_DECODE_DER_PRINT_DEBUG_INFO           0x10000000L  /* show DER item decoding debug info */
#define DS_DECODE_DER_PRINT_ASN_DEBUG_INFO       0x20000000L  /* show error, warning, and info messages within text (ASN or XML) output */

/* uFlags for DSDecodeDerFields() */

#define DS_DER_DECODEFIELDS_PACKET               0
#define DS_DER_DECODEFIELDS_BUFFER               1
#define DS_DER_DECODEFIELDS_OUTPUT_ASN           0x10
#define DS_DER_DECODEFIELDS_OUTPUT_XML           0x20 

/* error conditions for all APIs */

#define DECODE_FIELDS_ERROR_EXCEEDS_BUFLEN1      -2
#define DECODE_FIELDS_ERROR_EXCEEDS_BUFLEN2      -3
#define DECODE_FIELDS_ERROR_EXCEEDS_ITER_LIMIT1  -4
#define DECODE_FIELDS_ERROR_EXCEEDS_ITER_LIMIT2  -5
#define DECODE_FIELDS_ERROR_NEGATIVE_TAGLEN      -6
#define DECODE_FIELDS_ERROR_TAGLEN_EXCEEDS_MAX   -7
#define DECODE_FIELDS_ERROR_NEGATIVE_SETLEN      -8
#define DECODE_FIELDS_ERROR_SETLEN_EXCEEDS_MAX   -9
#define DECODE_FIELDS_ERROR_CONSEC_LONGFORM_TAGS -10


/* ASN.1 tag definitions */

#ifdef _DERLIB_INSTALLED_  /* use OpenLI tag definitions if available */

  #define DER_TAG_CLASS_APPLICATION_PRIMITIVE    WANDDER_CLASS_APPLICATION_PRIMITIVE

  #define DER_TAG_BOOLEAN                        WANDDER_TAG_BOOLEAN
  #define DER_TAG_INTEGER                        WANDDER_TAG_INTEGER
  #define DER_TAG_OID                            WANDDER_TAG_OID
  #define DER_TAG_OCTETSTRING                    WANDDER_TAG_OCTETSTRING
  #define DER_TAG_NULL                           WANDDER_TAG_NULL
  #define DER_TAG_OBJECTDESCRIPTOR               WANDDER_TAG_OBJDESC
  #define DER_TAG_REAL                           WANDDER_TAG_REAL
  #define DER_TAG_ENUM                           WANDDER_TAG_ENUM
  #define DER_TAG_UTF8STRING                     WANDDER_TAG_UTF8STR
  #define DER_TAG_SEQUENCE                       WANDDER_TAG_SEQUENCE
  #define DER_TAG_SET                            WANDDER_TAG_SET
#else

  #define DER_TAG_CLASS_APPLICATION_PRIMITIVE    2

  #define DER_TAG_BOOLEAN                        1
  #define DER_TAG_INTEGER                        2
  #define DER_TAG_OCTETSTRING                    4
  #define DER_TAG_NULL                           5
  #define DER_TAG_OID                            6
  #define DER_TAG_OBJECTDESCRIPTOR               7
  #define DER_TAG_REAL                           9
  #define DER_TAG_ENUM                           10
  #define DER_TAG_UTF8STRING                     12
  #define DER_TAG_SEQUENCE                       16
  #define DER_TAG_SET                            17
#endif

#define DER_TAG_EXTERNAL                         8
#define DER_TAG_CLASS_CONSTRUCT                  1

#define DER_TAG_INTERCEPTPOINTID                 ((DER_TAG_CLASS_APPLICATION_PRIMITIVE << 6) | DER_TAG_OID)
#define DER_TAG_SEQNUM                           ((DER_TAG_CLASS_APPLICATION_PRIMITIVE << 6) | DER_TAG_OCTETSTRING)
#define DER_TAG_TIMESTAMP                        ((DER_TAG_CLASS_APPLICATION_PRIMITIVE << 6) | (DER_TAG_CLASS_CONSTRUCT << 5) | DER_TAG_OBJECTDESCRIPTOR)
#define DER_TAG_TIMESTAMPQUALIFIER               ((DER_TAG_CLASS_APPLICATION_PRIMITIVE << 6) | DER_TAG_EXTERNAL)

#ifdef __cplusplus
extern "C" {
#endif

  extern const char DERLIB_VERSION[256];  /* derlib version string */

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

     -has to be called once at app init time, by only one application thread
     -uFlags options given by DS_CD_XX items above
     -return value < 0 indicates an error condition
  */

  int DSConfigDerlib(GLOBAL_CONFIG* pGlobalConfig, DEBUG_CONFIG* pDebugConfig, unsigned int uFlags);

  /* DSFindDerStream() finds and/or auto-detects DER encoded streams and destination ports for existing streams

     -pkt_in_buf should contain a standard IPv4 or IPv6 TCP/IP packet, including header(s) and payload
     -uFlags options given by DS_DER_FIND_XX items above
     -if uFlags includes DS_DER_FIND_INTERCEPTPOINTID:
       -if specified, then szInterceptPointId contains an interception point Id upon successful return
       -if not specified, then szInterceptPointId must contain a valid string
     -upon successful return, dest_port_list[] contains one or more destination ports associated with the interception point Id. The port list is zero-terminated
     -return value < 0 indicates an error condition
  */

  int DSFindDerStream(uint8_t* pkt_in_buf, unsigned int uFlags, char* szInterceptPointId, uint16_t dest_port_list[], FILE* hFile_xml_output);

  /* DSCreateDerStream() creates a DER stream

     -returns a handle to a new DER stream
     -szInterceptPointId must contain a valid string, either user-supplied or determined by prior call to DSDetectDerStream()
     -hFile_xml_output should be 1) a handle to a text file opened for writing or 2) NULL if not used
     -return value < 0 indicates an error condition
  */

  HDERSTREAM DSCreateDerStream(const char* szInterceptPointId, uint16_t intercept_dest_port, unsigned int uFlags);

  /* DSGetDerStreamInfo() retrieves DER stream info

     -hDerStream must be a DER stream handle created by a prior call to DSCreateDerStream()
     -uFlags options given by DS_DER_INFO_XX items above
     -return value:
       port, index, packet count, etc (uFlags DS_DER_INFO_DSTPORT, DS_DER_INFO_ASN_INDEX, DS_DER_INFO_CC_PKT_COUNT, etc)
       1 for info returned in pInfo (uFlags DS_DER_INFO_INTERCEPTPOINTID, DS_DER_INFO_DSTPORT_LIST, etc)
       < 0 indicates an error condition
  */

  int64_t DSGetDerStreamInfo(HDERSTREAM hDerStream, unsigned int uFlags, void* pInfo);

  /* DSSetDerStreamInfo() sets DER stream info

     -hDerStream must be a DER stream handle created by a prior call to DSCreateDerStream()
     -uFlags options given by DS_DER_INFO_XX items above
     -pInfo:
       pointer to port and port value (uFlag DS_DER_INFO_DSTPORT), pointer to port list (uFlag DS_DER_INFO_DSTPORT_LIST)
     -return value:
       > 0 indicates success
       < 0 indicates an error condition
  */

  int64_t DSSetDerStreamInfo(HDERSTREAM hDerStream, unsigned int uFlags, void* pInfo);

  /* DSDecodeDerStream() decodes a DER encoded stream, returning one or more decoded items

     -hDerStream must be a DER stream handle created by a prior call to DSCreateDerStream()
     -pkt_in_buf should contain a standard IPv4 or IPv6 TCP/IP packet, including header(s) and payload
     -uFlags options given by DS_DER_XX items above
     -der_decode should point to an HI3_DER_DECODE struct to be filled in with decoded items
     -hFile_xml_output should be 1) a handle to a text file opened for writing or 2) NULL if not used
     -return value:
         0 - nothing found
       > 0 - length of CC packet found
       < 0 - error condition

     Notes:

      -DSDecodeDerStream() uses a heuristic approach to locate interception point IDs and decode encapsulated IP/UDP/RTP packets, while DSDecodeDerFields() strictly follows ETSI TS 102 232-x ASN.1 specs. If hFile_xml_output is provided, DSDecodeDerStream() calls DSDecodeDerFields() and the two methods operate concurrently (i.e. RTP decoding and ASN.1 decoding)

      -DSDecodeDerStream() handles packet aggregation; i.e. re-assembling data split across packet boundaries
  */

  int DSDecodeDerStream(HDERSTREAM hDerStream, uint8_t* pkt_in_buf, uint8_t* pkt_out_buf, unsigned int uFlags, HI3_DER_DECODE* der_decode, FILE* hFile_xml_output);

  /* DSDeleteDerStream() deletes a DER stream

     -hDerStream must be a DER stream handle created by a prior call to DSCreateDerStream()
     -return value < 0 indicates an error condition
  */

  int DSDeleteDerStream(HDERSTREAM hDerStream);

/* DSDecodeDerFields() decodes one or more DER fields

  -pointer to buffer or packet
  -uFlags
    -DS_DER_DECODEFIELDS_PACKET indicates buffer contains a TCP/IP packet (default if not specified)
    -DS_DER_DECODEFIELDS_BUFFER indicates raw byte buffer without packet or other header
  -plen
    -0 --> decode field(s) pointed to (i.e. one field or a set containing sub fields), starting with tag
    -non-zero --> decode as many fields as contained in plen. Processing starts on first valid tag found
  -optional XML output file handle to a text file opened for writing
  -optional tag label, mainly useful for individual fields

  -returns bytes processed on success, -1 on error condition
*/

int DSDecodeDerFields(uint8_t* p, unsigned int uFlags, int plen, FILE* hFile, const char* label);

#ifdef __cplusplus
}
#endif

#endif
