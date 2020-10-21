/*
  $Header: /root/Signalogic_YYYYvN/shared_include/net.h
 
  Purpose:
 
    Network related API, struct and constant definitions

  Description
  
    Shared header file between host and target

  Copyright (C) Signalogic Inc. 2015
 
  Revision History:
 
    Created,  18May15, JHB
    Modified, 2Mar16, CJ - Added defines and enum for host-c66x network control
  
*/

#ifndef CIM_NET_H
#define CIM_NET_H


#ifdef __cplusplus
  extern "C" {
#endif


/* network I/O initialization status codes */
/* Statuses are used as bit fields so multiple statuses may be set at a time */
#define NETWORK_IO_INIT_COMPLETE             0x1
#define NETWORK_IO_CABLE_DISCONNECTED        0x2

/* network I/O initialization error codes */

#define NETWORK_IO_INIT_TIMEOUT              (-1)
#define NETWORK_IO_INIT_GENERAL_FAILURE      (-2)
#define NETWORK_IO_INIT_QMSS_FAILURE         (-3)
#define NETWORK_IO_INIT_CPPI_FAILURE         (-4)
#define NETWORK_IO_INIT_PASS_FAILURE         (-5)
#define NETWORK_IO_INIT_CPSW_FAILURE         (-6)
#define NETWORK_IO_SETUP_TX_FAILURE          (-7)
#define NETWORK_IO_INIT_QMSS_LOCAL_FAILURE   (-8)
#define NETWORK_IO_SETUP_RX_FAILURE          (-9)
#define NETWORK_IO_SETUP_PASS_FAILURE        (-10)
#define NETWORK_IO_INIT_PHY_ACCESS_FAILURE   (-11)


/* Reserved in the RTSC platform for host-c66x network control, 16-bytes length */
#define NET_INIT_CONTROL      0x0C3FFEF0
#define NET_INIT_CONTROL_LEN  16

enum {
   NET_IO_ACTIVE,
   NET_IO_PAUSE_CMD,
   NET_IO_PAUSE_ACK,
   NET_IO_PAUSE_COM,
   NET_IO_RES_CMD,
   NET_IO_RES_COM,
   NET_IO_INACTIVE
};

#ifdef __cplusplus
}
#endif


#endif /* CIM_NET_H */
