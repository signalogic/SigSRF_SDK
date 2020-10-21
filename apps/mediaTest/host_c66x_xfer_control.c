/*
 $Header: /root/Signalogic/DirectCore/apps/SigC641x_C667x/mediaTest/mailBox_interface.c

 Description:
 
   Functions for controlling data transfers between C66x and host via host-mapped memory
 
 Copyright (C) Signalogic Inc. 2015-2019
 
 Revision History:
 
   Created, CJ, September 2015
*/

#include "mediaTest.h"

void fill_pcie_buffer(uint8_t *buffer, int length, uint32_t chip_id, uint32_t core_id)
{
   fp_buffers_t *ptr;
   uint8_t *filled_buffer;

   ptr = &fp_buffers[chip_id][core_id];
   filled_buffer = (ptr->curr_buffer_index == 0) ? ptr->dp_dsp_buffer_a : ptr->dp_dsp_buffer_b;

   if ((ptr->curr_buffer_length + length) < DP_DSP_BUFFER_SIZE)
   {
      memcpy(filled_buffer + ptr->curr_buffer_length, buffer,length);
      ptr->curr_buffer_length += length;
   }
   else
      printf("Buffer is full. Drop packet\n");
}

//static int send_data = 0;

static void check_for_single_dp_dsp_xfer(uint32_t chip_id, uint32_t core_id)
{
   fp_buffers_t *ptr;

   ptr = &fp_buffers[chip_id][core_id];

   // Data transfer from DP to DSP
   //while ((*ptr->dp_dsp_ctrl_reg & CTRL_DP_DSP_NEED_DATA) != CTRL_DP_DSP_NEED_DATA);
   if ((*ptr->dp_dsp_ctrl_reg & CTRL_DP_DSP_NEED_DATA) == CTRL_DP_DSP_NEED_DATA)
   {
      if (ptr->curr_buffer_length > 0)
      {
         //printf("\n+++++ CTRL_DP_DSP_NEED_DATA\n");
         //printf("send data count = %d, chip_id = %d, core_id = %d\n", send_data++, chip_id, core_id);
         // Set up transfer info
         
         *ptr->dp_dsp_length = ptr->curr_buffer_length;
         *ptr->dp_dsp_buffer_id = ptr->curr_buffer_index;

         // Switch buffer
         ptr->curr_buffer_index = (ptr->curr_buffer_index == 0) ? 1 : 0;
         ptr->curr_buffer_length = 0;

         // Tell DSP to get the data
         *ptr->dp_dsp_ctrl_reg = CTRL_DP_DSP_DATA_READY;
      }
   }
}

void check_for_host_to_c66x_xfer()
{
   uint32_t i;
   QWORD nCoreList_temp = nCoreList;

   for (i = 0; nCoreList_temp > 0; i++, nCoreList_temp >>= 1)
      if (nCoreList & (1 << i))
         check_for_single_dp_dsp_xfer(i/8, i%8);
}

/* check_for_single_dsp_dp_xfer() may need to add alignment size retuned by process_transcoded_packet(), CKJ Aug2017

   Notes:

   1) It seems that c66x xfer functions may need to account for alignment, in order to advance shared mem buffer pointers to match what c66x is doing

   2) Compare with, and see additional comments in c66x file transcode_processing_task.c (look for #ifdef _EVS_)

   3) This is a temporary fix, tested with EVS only.  It may not work with some packet lengths and/or with other codecs   
*/

//static int recv_count = 0;

static void check_for_single_dsp_dp_xfer(uint32_t chip_id, uint32_t core_id, int (*process_buffer)(unsigned char *, unsigned int))
{
   fp_buffers_t *ptr;
   uint32_t length, processed_length;
   int packet_length;
   uint8_t *buffer;

   ptr = &fp_buffers[chip_id][core_id];

   // Data transfer from DSP to DP
   if ((*ptr->dsp_dp_ctrl_reg & CTRL_DSP_DP_XFER_DONE) == CTRL_DSP_DP_XFER_DONE)
   {
      //printf("\n+++++ CTRL_DSP_DP_XFER_DONE\n");
      // Get data transfer info
      length = *ptr->dsp_dp_length;
      buffer = ptr->dsp_dp_buffer;
      if (length == 0)
         return;

      // Process data
      processed_length = 0;
      while (processed_length < length)
      {
         //printf("***** Transcoded packet recvd. dsp %d core %d, count = %d\n", chip_id, core_id, recv_count++);
         packet_length = process_buffer(buffer, length - processed_length);

         if (packet_length == -1)
         {
            printf("ERROR: processing packet buffer failed\n");
            break;
         }
         processed_length += packet_length;
         buffer += packet_length;
      }
      // Set the bit for next round
      *ptr->dsp_dp_ctrl_reg = CTRL_DSP_DP_CAN_XFER;
   }
}

void check_for_c66x_to_host_xfer(int (*process_buffer)(unsigned char *, unsigned int))
{
   uint32_t i;
   QWORD nCoreList_temp = nCoreList;

   for (i = 0; nCoreList_temp > 0; i++, nCoreList_temp >>= 1)
      if (nCoreList & (1 << i))
         check_for_single_dsp_dp_xfer(i/8, i%8, process_buffer);
}

