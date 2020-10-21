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

#ifndef _MAILBOX_H
#define _MAILBOX_H

/* add HCARD params and calling arguments to all mailbox functions for x86 builds w/wo coCPU.  This removes a global HCARD var from voplib, which is both better software design and also allows voplib
   to be used with minimum user apps without having to declare an artificial HCARD global var.  MailBoxLoc.h and mailBox.c are also modified to use card_param and card_arg.  JHB Aug 2018 */

#if _X86
  #define card_param HCARD hCard,
  #define card_arg hCard,
  #define HCARD int32_t
#else
  #define card_param  /* for non-x86 builds (e.g. c66x), definitions have no effect */
  #define card_arg
#endif

/* Mailbox is meant to exchange message between the Host and individual DSP 
   cores.  A mailbox is unidirectional either host -> DSP or DSP to Host */
/* Mailbox can also be used to send messages from DSP to DSP, but still each
 * mailbox will be unidirectional DSP1 -> DSP2 or DSP2 -> DSP1. Bidirectional
 * communication requires 2 mailboxes one on each direction.
 */

typedef struct mailBox_config_s
{
  uint32_t mem_start_addr;     /* Memory start address */
  uint32_t mem_size;           /* Size of memory space allocated for the mailbox */
  uint32_t max_payload_size;   /* Maximum Payload size */
} mailBox_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief Function mailBox_get_size() Get size needed for Mailbox instance
 *  @retval        size in bytes needed for mailbox instance
 *  @pre
 *  @post
 */
uint32_t mailBox_get_alloc_size(void);

/**
 *  @brief Function mailBox_get_mem_size() Get size needed for Mailbox memory
 *  @param[in]     max_payload_size   Maximum size of a mailBox message.
 *  @param[in]     mailBox_depth      Maximum number of messages that the 
 *                                    mailBox can hold at one time.
 *  @retval        size in bytes needed for mailbox memory
 *  @pre
 *  @post
 */
uint32_t mailBox_get_mem_size(uint32_t max_payload_size, uint32_t mailBox_depth);

/* Node id of  mail box for DSP should be set to
 * MAILBOX_MAKE_DSP_NODE_ID(dsp_id,core_id).
 * Node id of mail box for Host should be set to
 * MAILBOX_MAKE_HOST_NODE_ID(host_id)
 */
#define MAILBOX_ID_HOST_MASK     0x80000000

#define MAILBOX_MAKE_HOST_NODE_ID(host_id) (host_id | MAILBOX_ID_HOST_MASK)
/* TODO: Need to add notes on dsp_id & core id */
/* explain mulitple mailboxes use same writer and reader node id */
#define MAILBOX_MAKE_DSP_NODE_ID(dsp_id,core_id) ((dsp_id<< 8) | core_id)

#define MAILBOX_GET_DSP_ID(node_id) ((node_id >> 8) & 0xffffff)

/* LOCATION: Indicates whether the mailbox is located in the local or remote memory */
#define MAILBOX_MEMORY_LOCATION_LOCAL 0
#define MAILBOX_MEMORY_LOCATION_REMOTE 1

/* DIRECTION: Indicates from the local perspective whether the mailbox is used to
 * send messages or Receive messages
 */
#define MAILBOX_DIRECTION_RECEIVE  0
#define MAILBOX_DIRECTION_SEND     1

/**
 *  @brief Function mailBox_create() Creates a mailBox
 *  @param[out]    mailBoxHandle  Returned mailbox handle pointer
 *  @param[in]     remote_node_id Node id of remote node
 *  @param[in]     mem_location   memory location local or remote
 *  @param[in]     direction      send or receive
 *  @param[in]     mailBox_config MailBox configuration
 *  @retval        0 for success, -1 for failure
 *  @pre  
 *  @post 
 */
int32_t mailBox_create(card_param void *mailBoxHandle, int32_t remote_node_id,
  uint32_t mem_location, uint32_t direction, mailBox_config_t *mailBox_config);

/**
 *  @brief Function mailBox_open() Opens a mailBox; This is a blocking call, wait till the remote is ready
 *  @param[in]     mailBoxHandle  mailBox Handle
 *  @retval        0 for success, -1 for failure
 *  @pre
 *  @post
 */
int32_t mailBox_open(card_param void *mailBoxHandle);

#define MAILBOX_ERR_FAIL          -1
#define MAILBOX_ERR_MAIL_BOX_FULL -2
#define MAILB0X_ERR_EMPTY         -3
#define MAILBOX_READ_ERROR        -4


/**
 *  @brief Function mailBox_write() Writes into a mailBox to deliver to remote : Non blocking call
 *  @param[in]     mailBoxHandle  mailBox Handle
 *  @param[in]     *buf          Mailbox Payload buffer pointer
 *  @param[in]     size          Mailbox Payload buffer size
 *  @param[in]     trans_id      transaction ID for the mail
 *  @retval        0 for success, -1 for failure -2 FULL
 *  @pre  
 *  @post 
 */
int32_t mailBox_write(card_param void *mailBoxHandle, uint8_t *buf, uint32_t size, uint32_t trans_id);

/**
 *  @brief Function mailBox_read() Reads from a mailBox. This is a non blocking call
 *  @param[in]     mailBoxHandle  mailBox Handle
 *  @param[in]     *buf          Mailbox Payload buffer pointer
 *  @param[in]     *size         Mailbox Payload buffer size
 *  @param[in]     *trans_id     transaction ID for the mail
 *  @retval        0 for success, -1 for failure -3 Empty
 *  @pre  
 *  @post 
 */
int32_t mailBox_read(card_param void *mailBoxHandle, uint8_t *buf, uint32_t *size, uint32_t *trans_id);

/**
 *  @brief Function mailBox_query() Polls mailBoxes for any available messages. Non-blocking
 *  @param[in]     mailBoxHandle  mailBox Handle
 *  @retval        Number of messages in mailbox; negative error on failure
 *  @pre  
 *  @post 
 */
int32_t mailBox_query(card_param void *mailBoxHandle);

#ifdef __cplusplus
}
#endif

#endif /* _MAILBOX_H */
