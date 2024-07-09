/*
 *  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or
 *  code or tables extracted from it, as desired without restriction.

 $Header: /root/Signalogic/apps/mediaTest/mediaMin/common/crc/crc.h

 Copyright (C) Signalogic Inc. 2024

 License

 Revision History

   Created Jun 2024 JHB
*/

#ifndef _CRC_IO_H_
#define _CRC_IO_H_

int32_t calculate_crc32c(uint32_t crc32c, const unsigned char *buffer, unsigned int length);  /* in crc32.c */
#define crc32 calculate_crc32c

#endif
