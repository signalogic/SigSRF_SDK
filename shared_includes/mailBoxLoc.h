/*
 *
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/


/* see comments in mailBox.h regarding card_param and card_arg, JHB Aug 2018 */


#ifndef _MAILBOX_LOC_H
#define _MAILBOX_LOC_H

#include "stdint.h"
#include "mailBox.h"

#define MAILBOX_SLOT_OWNER_LOCAL    0xBABEFACE
#define MAILBOX_SLOT_OWNER_REMOTE   0xC00FFEEE

#define MAILBOX_NODE_ID_2_CORE_ID(id)         ((uint32_t)(id  & 0xff))
#define MAILBOX_NODE_ID_2_DSP_ID(id)          ((uint32_t)((id >>8)& 0x7fffff))

typedef struct mailBoxSlotHeader_s {
  uint32_t trans_id;             /* transaction id   */
  uint32_t payloadSize;          /* size of payload */
  uint32_t owner;                /* owner code */
} mailBoxSlotHeader_t;

typedef struct mailBoxSlot_s {
  mailBoxSlotHeader_t slotheader;    /* Slot header */
  uint8_t  mailboxPayload[1];    /* Number of payload bytes set to dummy 1: Actual size dynamically configured */
} mailBoxSlot_t;

typedef struct mailBoxHeader_s {
  uint32_t owner_code;           /* Owner code Local or remote */
  uint32_t writeIndex;           /* Write index */
  uint32_t readIndex;            /* Read index */
} mailBoxHeader_t;

typedef struct mailBox_s {
  mailBoxHeader_t mailboxheader;
  mailBoxSlot_t slots[1];      /* Number of slots set to dummy 1: Actual size dynamically configured  */
} mailBox_t;

typedef struct mailBoxInst_s {
  uint32_t mem_location;       /* Location of memory local or remote */
  uint32_t direction;          /* Direction: Send or recieve from local perspective */
  uint32_t mem_start_addr;     /* Location of mailbox */
  uint32_t mem_size;           /* size of Mailbox */
  uint32_t max_payload_size;   /* Maximum payload size */
  uint32_t remote_node_id;     /* Remote Node id */
  uint32_t depth;              /* Depth of mailbox */
  uint32_t slot_size;          /* Size of one mailbox slot */
  uint32_t writeCounter;       /* Number of writes to mailbox: Applicable only to Send mailbox   */
  uint32_t readCounter;        /* Number of reads to mailbox: Applicable only to receive mailbox */
} mailBoxInst_t ;

typedef struct mailBoxContext_s {
  uint32_t local_node_id;       /* Local node id */
}mailBoxContext_t;

void mailBoxNotify(card_param mailBoxInst_t *inst);

int32_t dsp_memory_read(card_param int32_t dsp_id, uint32_t addr, void *buf, uint32_t size);
int32_t dsp_memory_write(card_param int32_t dsp_id, uint32_t addr, void *buf, uint32_t size);

#endif /* _MAILBOX_LOC_H */

/*** nothing past this point ***/

