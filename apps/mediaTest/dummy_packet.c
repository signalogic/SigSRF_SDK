/*
 $Header: /root/Signalogic/DirectCore/apps/SigC641x_C667x/mediaTest/dummy_packet.c

 Description:
 
   Dummy PCMU packet for use with simulation testing.
 
 Copyright (C) Signalogic Inc. 2016
 
 Revision History:
 
   Created, CJ, January 2016
*/

//#include "mediaTest.h"

unsigned char dummy_packet[] = {    /* Ethernet header is unused */
                           //0x10, 0x11, 0x12, 0x13, 0x14, 0x15,                      /* Dest MAC */
                           //0x00, 0x01, 0x02, 0x03, 0x04, 0x05,                      /* Src MAC */
                           //0x08, 0x00,                                              /* Ethertype = IPv4 */
                           0x45, 0x00, 0x00, 0xc8,                                  /* IP version, services, total length */
                           0x00, 0x00, 0x00, 0x00,                                  /* IP ID, flags, fragment offset */
                           0x40, 0x11, 0x00, 0x00,                                  /* IP ttl, protocol (UDP), header checksum */
                           0x0a, 0x00, 0x00, 0x09,                                  /* Source IP address */
                           0x0a, 0x00, 0x00, 0x0a,                                  /* Destination IP address */
                           0x28, 0x00, 0x28, 0x00,                                  /* UDP source port, dest port */
                           0x00, 0xb4, 0x00, 0x00,                                  /* UDP len, UDP checksum */
                           0x80, 0x00, 0x00, 0x00,                                  /* RTP version, pading, extension, CSRC, marker, PT = PCMU, seq num */
                           0x00, 0x00, 0x00, 0x00,                                  /* RTP timestamp */
                           0x00, 0x00, 0x00, 0x00,                                  /* RTP SSRC */
                           0xec, 0xef, 0xf2, 0xed, 0xe9, 0xea, 0xf0, 0xef,          /* 160 byte RTP payloade */
                           0xe9, 0xe1, 0xdb, 0xd6, 0xd1, 0xce, 0xcc, 0xcc,
                           0xce, 0xd1, 0xd7, 0xd8, 0xd9, 0xdd, 0xdf, 0xe1,
                           0xe0, 0xdc, 0xd9, 0xd7, 0xd4, 0xcf, 0xd3, 0xda,
                           0xe3, 0xf5, 0x6f, 0x5e, 0x5a, 0x5a, 0x59, 0x58,
                           0x58, 0x59, 0x59, 0x5d, 0x62, 0x62, 0x5f, 0x5d,
                           0x59, 0x56, 0x56, 0x54, 0x4f, 0x4e, 0x4c, 0x4b,
                           0x4c, 0x4b, 0x4c, 0x4f, 0x57, 0x5e, 0x68, 0x73,
                           0xfe, 0xef, 0xec, 0xf4, 0x7e, 0x72, 0x76, 0x73,
                           0x75, 0xfb, 0xe9, 0xdc, 0xd5, 0xd0, 0xcf, 0xce,
                           0xcb, 0xcb, 0xcd, 0xcf, 0xd3, 0xd5, 0xd7, 0xda,
                           0xde, 0xde, 0xdb, 0xd6, 0xd7, 0xd7, 0xd7, 0xd9,
                           0xd7, 0xd4, 0xd4, 0xd5, 0xd3, 0xd5, 0xdf, 0xf8,
                           0x6c, 0x5c, 0x54, 0x4e, 0x4c, 0x4c, 0x4e, 0x50,
                           0x53, 0x5b, 0x65, 0x6d, 0x70, 0x69, 0x5f, 0x58,
                           0x50, 0x4b, 0x48, 0x47, 0x49, 0x4b, 0x4d, 0x4e,
                           0x56, 0x5e, 0x64, 0x6b, 0x72, 0xfd, 0xef, 0xe6,
                           0xe3, 0xe2, 0xde, 0xdb, 0xd8, 0xdb, 0xdb, 0xd7,
                           0xd3, 0xd1, 0xcf, 0xcd, 0xca, 0xc9, 0xcb, 0xca,
                           0xcb, 0xcd, 0xce, 0xd2, 0xd9, 0xde, 0xe3, 0xea };

unsigned int sizeof_dummy_packet = sizeof(dummy_packet);
