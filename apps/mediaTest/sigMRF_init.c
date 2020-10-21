/*
 $Header: /root/Signalogic/DirectCore/apps/SigC641x_C667x/mediaTest/sigMRF_init.c

 Description:
 
   Functions for initializing C66x media transcoding software (SigMRF) once 
      the software has been loaded and has started running
 
 Copyright (C) Signalogic Inc. 2015-2018
 
 Revision History:
 
   Created CJ, September 2015
   Modified JHB Mar 2017, edits for codec testing
   MOdified JHB Aug 2018, add hCard args to mailbox calls.  See changes in shared_include header files mailBox.h and MailBoxLoc.h
*/

#include "mediaTest.h"
#include "mailBoxLoc.h"

/* This parameter is used to configure the CMEM host pools for use with
   mapping to coCPU Memory range; should be 1MB, 2MB, 4MB or 8MB */
#define C66X_OB_REGION_SIZE                0x400000


extern HCARD hCard;  /* handle to coCPU card.  Defined in mediaTest.c */


enum alloc
{
   CMEM_ALLOC = 0x01,
   OUTBOUND_ALLOC = 0x02
};

typedef struct hostmem_s
{
   char allocated;                  // bit field for what has been allocated, use with alloc enum
   host_buf_desc_t buf_desc;   // cmem module buffer descriptor
   unsigned int c66x_addr;          // C66x address for shared mem region
   
} hostmem_t;

extern char codec_test;

hostmem_t hostmem[MAXCPUS][MAXCORESPERCPU];
fp_buffers_t fp_buffers[MAXCPUS][MAXCORESPERCPU];
static dsp_config_info_t dsp_config_info[MAXCPUS][MAXCORESPERCPU];

/* mailbox locations */
static unsigned int dsp2hostmailbox, host2dspmailbox;

/* Stores rx and tx mail ids for all the cores */
static void *rx_mailbox_handle[MAXCPUS*MAXCORESPERCPU],
            *tx_mailbox_handle[MAXCPUS*MAXCORESPERCPU];

// Update C66x addresses and host pointers
void update_config_info(int chip, int core)
{
   // Set C66x config info
   dsp_config_info[chip][core].dp_dsp_ctrl_reg = hostmem[chip][core].c66x_addr;
   dsp_config_info[chip][core].dsp_dp_ctrl_reg = dsp_config_info[chip][core].dp_dsp_ctrl_reg + sizeof(unsigned int);
   dsp_config_info[chip][core].dp_dsp_length = dsp_config_info[chip][core].dsp_dp_ctrl_reg + sizeof(unsigned int);
   dsp_config_info[chip][core].dp_dsp_buffer_id = dsp_config_info[chip][core].dp_dsp_length + sizeof(unsigned int);
   dsp_config_info[chip][core].dsp_dp_length = dsp_config_info[chip][core].dp_dsp_buffer_id + sizeof(unsigned int);
   dsp_config_info[chip][core].dp_dsp_buffer_a = dsp_config_info[chip][core].dsp_dp_length + sizeof(unsigned int);
   dsp_config_info[chip][core].dp_dsp_buffer_b = dsp_config_info[chip][core].dp_dsp_buffer_a + DP_DSP_BUFFER_SIZE;
   dsp_config_info[chip][core].dsp_dp_buffer = dsp_config_info[chip][core].dp_dsp_buffer_b + DP_DSP_BUFFER_SIZE;

   // Set host pointers and initialize
   fp_buffers[chip][core].dp_dsp_ctrl_reg = (unsigned int *) hostmem[chip][core].buf_desc.userAddr;
   *fp_buffers[chip][core].dp_dsp_ctrl_reg = 0;
   fp_buffers[chip][core].dsp_dp_ctrl_reg = (unsigned int *) (((unsigned char *) fp_buffers[chip][core].dp_dsp_ctrl_reg) + sizeof(unsigned int));
   *fp_buffers[chip][core].dsp_dp_ctrl_reg = CTRL_DSP_DP_CAN_XFER;
   fp_buffers[chip][core].dp_dsp_length = (unsigned int *) (((unsigned char *) fp_buffers[chip][core].dsp_dp_ctrl_reg) + sizeof(unsigned int));
   *fp_buffers[chip][core].dp_dsp_length = 0;
   fp_buffers[chip][core].dp_dsp_buffer_id = (unsigned int *) (((unsigned char *) fp_buffers[chip][core].dp_dsp_length) + sizeof(unsigned int));
   *fp_buffers[chip][core].dp_dsp_buffer_id = 0;
   fp_buffers[chip][core].dsp_dp_length = (unsigned int *) (((unsigned char *) fp_buffers[chip][core].dp_dsp_buffer_id) + sizeof(unsigned int));
   *fp_buffers[chip][core].dsp_dp_length = 0;
   fp_buffers[chip][core].dp_dsp_buffer_a = ((unsigned char *) fp_buffers[chip][core].dsp_dp_length) + sizeof(unsigned int);
   fp_buffers[chip][core].dp_dsp_buffer_b = ((unsigned char *) fp_buffers[chip][core].dp_dsp_buffer_a) + DP_DSP_BUFFER_SIZE;
   fp_buffers[chip][core].dsp_dp_buffer = ((unsigned char *) fp_buffers[chip][core].dp_dsp_buffer_b) + DP_DSP_BUFFER_SIZE;

   fp_buffers[chip][core].curr_buffer_index = 0;
   fp_buffers[chip][core].curr_buffer_length = 0;
}

// Write configuration info to each chip
// Return 0 if successful, -1 if error
int write_config_info(int chip)
{
   int core, chip_addr;
   unsigned int magic = MAGIC_NUMBER;
   static unsigned int config_info_addr;
   
   /* Get C66x config info address to communicate the init info */
   if ((config_info_addr = DSGetSymbolAddr(hCard, NULL, "dsp_config_info")) == 0)
   {
      printf("ERROR: failed to get config info address\n");
      return -1;
   }
   //printf("Config info address = 0x%x\n", config_info_addr);

   chip_addr = config_info_addr + sizeof(unsigned int);
   for (core = 0; core < (int)nCoresPerCPU; core++)
   {
      if (DSWriteMemEx(hCard, DS_GM_LINEAR_DATA, chip_addr, DS_GM_SIZE32, (unsigned char *)&dsp_config_info[chip][core], sizeof(dsp_config_info_t)/4, 1 << (chip*8)) == 0)
         return -1;
      chip_addr += sizeof(dsp_config_info_t);
   }

   // Write magic number to c66x chip to let it start
   if (DSWriteMemEx(hCard, DS_GM_LINEAR_DATA, config_info_addr, DS_GM_SIZE32, (unsigned char *)&magic, sizeof(unsigned int)/4, 1 << (chip*8)) == 0)
      return -1;
      
   return 0;
}

static int hostmem_free(void)
{
   int i, chip, core;
   QWORD nCoreList_temp = nCoreList;
   
   for (i = 0; nCoreList_temp > 0; i++, nCoreList_temp >>= 1)
   {
      if (nCoreList & (1 << i))
      {
         chip = i/8; core = i%8;
         // Free contiguous dma host mem
         if (hostmem[chip][core].allocated & CMEM_ALLOC)
         {
            DSFreeHostContigMem(hCard, 1, DS_DYNAMIC_HOST_BUF, &hostmem[chip][core].buf_desc);
            hostmem[chip][core].allocated &= ~CMEM_ALLOC;
         }
            
         // Free C66x outbound mem range
         if (hostmem[chip][core].allocated & OUTBOUND_ALLOC)
         {
            DSFreeC66xAddr(hCard, C66X_OB_REGION_SIZE, hostmem[chip][core].c66x_addr);
            hostmem[chip][core].allocated &= ~OUTBOUND_ALLOC;
         }
      }
   }

   return 0;
}

static int hostmem_alloc(void)
{
   int i, chip, core;
   QWORD nCoreList_temp = nCoreList;
   
   memset(&hostmem, 0, sizeof(hostmem));
   memset(&fp_buffers, 0, sizeof(fp_buffers));
   memset(&dsp_config_info, 0, sizeof(dsp_config_info));

   for (i = 0; nCoreList_temp > 0; i++, nCoreList_temp >>= 1)
   {
      if (nCoreList & (1 << i))
      {
         DSSetCoreList(hCard, nCoreList & (1 << i));
         
         chip = i/8; core = i%8;
         
         // Allocate host contiguous memory
         if (DSAllocHostContigMem(hCard, 1, C66X_OB_REGION_SIZE, DS_DYNAMIC_HOST_BUF, &hostmem[chip][core].buf_desc) != 0)
         {
            printf("ERROR: chip = %d, core = %d: DSAllocHostContigMem() failed\n", chip, core);
            goto err_hostmem;
         }
         hostmem[chip][core].allocated |= CMEM_ALLOC;

         // Allocate C66x address
         if (DSAllocC66xAddr(hCard, C66X_OB_REGION_SIZE, &hostmem[chip][core].c66x_addr) != 0)
         {
            printf("ERROR chip = %d, core = %d: DSAllocC66xAddr() failed\n", chip, core);
            goto err_hostmem;
         }
         hostmem[chip][core].allocated |= OUTBOUND_ALLOC;
         
         //printf("chip = %d, core = %d: C66x address = 0x%x\n", chip, core, hostmem[chip][core].c66x_addr);

         // Map host memory to C66x address
         if (DSMapHostMemToC66xAddr(hCard, 1, &hostmem[chip][core].buf_desc, hostmem[chip][core].c66x_addr) != 0)
         {
            printf("chip = %d, core = %d: DSMapHostMemToC66xAddr() failed\n", chip, core);
            goto err_hostmem;
         }
         
         // Fill C66x address and pointers
         update_config_info(chip, core);
         
         // Write configuration info to C66x
         if ((i%8) == 0)
         {
            if (write_config_info(chip) != 0)
            {
               printf("ERROR: write_config_info() failed on chip %d\n", i/8);
               goto err_hostmem;
            }
         }
      }
   }
   
   DSSetCoreList(hCard, nCoreList);
   
   printf("Host-mapped shared memory allocation and initilization completed\n");
   return 0;
   
err_hostmem:
   hostmem_free();
   return -1;
}

#define SYNC_ADDRESS 0x00800300
static int host_c66x_sync(void)
{
   int rval = 0, wval = 0x9abcdef0, i = 0;
   QWORD nCoreList_temp = nCoreList;
   
   for (i = 0; nCoreList_temp > 0; i++, nCoreList_temp >>= 8)
   {
      if (nCoreList & (1 << (i*8)))
      {
         rval = 0;  
         while(rval != 0x12345678)
            DSReadMemEx(hCard, DS_GM_LINEAR_DATA, SYNC_ADDRESS, DS_RM_SIZE32, (unsigned char *)&rval, 1, 1 << i);
         
         DSWriteMemEx(hCard, DS_GM_LINEAR_DATA, SYNC_ADDRESS, DS_RM_SIZE32, (unsigned char *)&wval, 1, 1 << i);
         
         //printf("Host synced with chip %d\n", i);
      }
   }
   
   return 0;
}

static void mailbox_free(void)
{
   int chip;

   // Free all mailbox memory
   for (chip = 0; chip < (int)numCores; chip++)
   {
      if (rx_mailbox_handle[chip] != NULL)
         free(rx_mailbox_handle[chip]);
      if (tx_mailbox_handle[chip] != NULL)
         free(tx_mailbox_handle[chip]);
   }
}

static int mailbox_create(unsigned int node)
{
   unsigned int core = node % MAXCORESPERCPU;
   unsigned int chip = node / MAXCORESPERCPU;
   mailBox_config_t mailBox_config;

   // Allocate TX mailbox handle
   if (tx_mailbox_handle[node] == NULL)
   {
      tx_mailbox_handle[node] = malloc(sizeof(mailBoxInst_t));
      if (tx_mailbox_handle[node] == NULL)
      {
         printf("ERROR: Failed to allocate memory for TX mailbox on node = %d\n", node);
         return -1;
      }
   }
   
   // Create TX mailbox
   mailBox_config.mem_start_addr = host2dspmailbox + (core * TRANS_PER_MAILBOX_MEM_SIZE);
   mailBox_config.mem_size = TRANS_PER_MAILBOX_MEM_SIZE;
   mailBox_config.max_payload_size = TRANS_MAILBOX_MAX_PAYLOAD_SIZE;
   if (mailBox_create(hCard, tx_mailbox_handle[node], MAILBOX_MAKE_DSP_NODE_ID(chip, core),
      MAILBOX_MEMORY_LOCATION_REMOTE, MAILBOX_DIRECTION_SEND, &mailBox_config) != 0)
   {
      printf("ERROR: TX amilbox_create failed on node: %d\n", node);
      return -1;
   }
   
   // Allocate RX mailbox handle
   if (rx_mailbox_handle[node] == NULL)
   {
      rx_mailbox_handle[node] = malloc(sizeof(mailBoxInst_t));
      if (rx_mailbox_handle[node] == NULL)
      {
         printf("ERROR: Failed to allocate memory for RX mailbox on node = %d\n", node);
         return -1;
      }
   }
   
   // Create RX mailbox
   mailBox_config.mem_start_addr = dsp2hostmailbox + (core * TRANS_PER_MAILBOX_MEM_SIZE);
   if (mailBox_create(hCard, rx_mailbox_handle[node], MAILBOX_MAKE_DSP_NODE_ID(chip, core),
      MAILBOX_MEMORY_LOCATION_REMOTE, MAILBOX_DIRECTION_RECEIVE, &mailBox_config) != 0)
   {
      printf("RX mailBox_create() failed on node: %d\n", node);
      return -1;
   }
   
   // Open TX mailbox
   if (mailBox_open(hCard, tx_mailbox_handle[node]) != 0)
   {
      printf("ERROR: TX mailBox_open() failed on node = %d\n", node);
      return -1;
   }
   
   // Open RX mailbox
   if (mailBox_open(hCard, rx_mailbox_handle[node]) != 0)
   {
      printf("ERROR: RX mailBox_open() failed on node = %d\n", node);
      return -1;
   }

   return 0;
}

static int mailbox_init(void)
{
   unsigned int node;
   
   if ((host2dspmailbox = DSGetSymbolAddr(hCard, NULL, "host2dspmailbox")) == 0)
   {
      printf("ERROR: Unable to get symbol address for host2dspmailbox, addr = 0x%x\n", host2dspmailbox);
      return -1;
   }
   
   if ((dsp2hostmailbox = DSGetSymbolAddr(hCard, NULL, "dsp2hostmailbox")) == 0)
   {
      printf("ERROR: Unable to get symbol address for dsp2hostmailbox\n");
      return -1;
   }
   
   // Create and open mailboxes
   memset(tx_mailbox_handle, 0, sizeof(tx_mailbox_handle));
   memset(rx_mailbox_handle, 0, sizeof(rx_mailbox_handle));
   
   for (node = 0; node < numCores; node++)
   {
      //printf("Creating mailbox on node %d\n", node);
      if (mailbox_create(node) != 0)
      {
         printf("ERROR: failed to create mailbox for node: %d\n", node);
         return -1;
      }
   }

   printf("Create/open mailboxes completed\n");
   return 0;
}
   
int sigMRF_init(void)
{
   /* Initialize host-mapped memory */
   if (network_packet_test || cocpu_sim_test)
   {
      if (hostmem_alloc() == -1)
      {
         printf("ERROR: failed to initialize host-mapped memory\n");
         return -1;
      }
      else
         printf("Contiguous host mem allocated successfully\n");
   }
   
   /* Sync host and C66x */
   if (host_c66x_sync() != 0)
   {
      printf("ERROR: failed to sync\n");
      return -1;
   }
   else
      printf("Host and C66x CPUs synchronized\n");

/* try to clear mailbox_enable flag before c66x code reaches mailbox_init() -- Chris, where is correct place to do this */

   if (codec_test) {
   
      DWORD zero = 0;
      DWORD dw_mailbox_enable_addr = DSGetSymbolAddr(hCard, NULL, "mailbox_enable");

      if (dw_mailbox_enable_addr) {
      
         printf("Disabling mailbox_init\n");
         DSWriteMem(hCard, DS_GM_LINEAR_DATA, dw_mailbox_enable_addr, DS_GM_SIZE32, &zero, 1);
      }
   }
   
   usleep(1000 * 1000);    // 1 s
   
   /* Initialize mailboxes */
   if (!codec_test)
   {
      if (mailbox_init() != 0)
      {
         printf("ERROR: failed to initialize mailboxes\n");
         return -1;
      }
   }
   
   return 0;
}

void sigMRF_cleanup(void)
{
   if (!codec_test) mailbox_free();
   hostmem_free();
}

// Polls RX mailbox for any available messages to read (Non-blocking)
// Returns number of messages in mailbox; negative error on failure
int query_mb(unsigned int node)
{
   return mailBox_query(hCard, rx_mailbox_handle[node]);
}

// Reads from a mailBox. This is a blocking call
//    node        Unique ID of the mailBox
//    *buf        Mailbox Payload buffer pointer
//    *size       Mailbox Payload buffer size
//    *trans_id   transaction ID for the mail
// Returns 0 for success, -1 for failure
int read_mb(unsigned int node, unsigned char *buf, unsigned int *size, unsigned int *trans_id)
{
   return mailBox_read(hCard, rx_mailbox_handle[node], buf, size, trans_id);
}

// Writes into a mailBox to deliver to remote
//    node        Unique ID of the mailBox
//    *buf        Mailbox Payload buffer pointer
//    size        Mailbox Payload buffer size
//    trans_id    transaction ID for the mail
// Returns 0 for success, -1 for failure
int write_mb(unsigned int node, unsigned char *buf, unsigned int size, unsigned int trans_id)
{
   return mailBox_write(hCard, tx_mailbox_handle[node], buf, size, trans_id);
}

