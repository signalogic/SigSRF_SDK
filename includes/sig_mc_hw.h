/* $Header: /root/Signalogic_2012v4/DirectCore/include/sig_mc_hw.h
 *
 * Purpose / Description:
 * Header file for SigC6xxx hardware driver
 * Used by DirectCore libraries, may be used directly by host application programs if needed
 *
 * Copyright (C) Signalogic Inc. 2002-2017
 *
 * Project:  DirectCore Linux driver for SigC66xx quad and octal PCIe cards, SigC64xx PTMC modules, and 8901 ATCA C66x boards
 * 
 * 1     10/06/05 3:32p Nithin
 * 
 * 1     8/25/05 3:39p Rpiwowarski
 * Initial check-in

 * Revision History

     Created  May-Oct 2005, Ryan Piwowarski
     Modified 10/06/05 Nithin
     Modified Nov-Dec 2007 JHB, Fixed HPI32 issue when reading slow DSP mem (e.g. SDRAM)
     Modified Jun-Jul 2008 Yahia Tachwali, Added debug constructs during test on HW400c/2
     Modified Oct 2008 Yahia Tachwali, Added ioctl() support for TDM/CTbus registers
     Modified Nov 2008 JHB, added REG_TDM_BASE
     Modified Dec 2008 JHB, updated revision register info
     Modified Jun 2012 JHB, added Aggr FPGA register definitions
     Modified Aug-Nov 2012 CJ, added C66x support
     Modified Jun 2013 JHB, fixed scope problem with MAX_DRIVER_DEVICES
     Modified Jun 2014 AM, added 2GB DDR3 support for C66x devices
     Modified Oct 2014 SC, Added functionality for SRIO driver (Advantech 8901 ATCA board)
     Modified Jun 2015 JHB, Added TI667X_PCIEEP_GET_PCI_INFO case in sig_mc_hw ioctl function. Provides host app with ability to read PCI info, including primary and subsystem vendor and device IDs.
     Modified Jul 2015 CJ, Added support for virtual sig device driver
     Modified Aug 2015 CJ, Added file descriptor specific struct for allowing multiple processes to open and use the driver
     Modified Sep 2015 CJ, Added support for allocating contiguous host memory and mapping it to C66x addresses
     Modified Feb 2016 JHB, added  SIG_GET_PCI_COMMON_PARENT_BUS_NUM case to sig_mc_hw_ioctl()
     Modified Feb 2016 CJ, added support for 64-bit BAR window mode while maintaining 32-bit BAR window mode compatibility
     Modified Mar 2016 CJ, added direct chip access struct
     Modified Aug 2017 JHB, replaced C++ style comments with C style for compatibility with ISO C90 sources
*/

#ifndef __SIGC6X_DRIVER__
#define __SIGC6X_DRIVER__

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

/* Set the maximum number of devices that the driver will have to handle */
#define MAX_SIGC5561_DEVICES  16
#define MAX_SIGC6415_DEVICES  16
#define MAX_SIGC6678_DEVICES  128

/* Driver command line parameter, these bit flags may be combined as needed */

#define _NOPCI_   0x1  /* disable PCIe, SC Oct2014 */
#define _USESRIO_ 0x2  /* enable SRIO

  Usage examples:

   modprobe sig_mc_hw hwmode=2    no PCIe, enable SRIO
   modprobe sig_mc_hw hwmode=3    enable both PCIe and SRIO

 Default (if no entry) value of hwmode = 0
*/

/****************************************************************/
/* Defines shared by both driver code and user application code */
/****************************************************************/

/* Defines for ioctl() calls into the driver */

#define IO_MAGIC 0xD5   /* Sets the base of the IO defines */

#define SIGC6415_DEBUG0             _IO(IO_MAGIC,  0)
#define SIGC6415_DEBUG1             _IO(IO_MAGIC,  1)
#define SIGC6415_DEBUG2             _IO(IO_MAGIC,  2)
#define SIG_READ_MEM                _IO(IO_MAGIC, 10)
#define SIG_WRITE_MEM               _IO(IO_MAGIC, 11)
#define SIG_READ_REG                _IO(IO_MAGIC, 12)
#define SIG_WRITE_REG               _IO(IO_MAGIC, 13)
#define SIG_SET_READ_MODE           _IO(IO_MAGIC, 14)
#define SIG_SET_WRITE_MODE          _IO(IO_MAGIC, 15)
#define SIGC6415_SET_CHUNK_SIZE     _IO(IO_MAGIC, 16)
#define SIGC6415_READ_TDM_WP        _IO(IO_MAGIC, 17)  /* Cmd for Reg read in Bar 2 */
#define SIGC6415_WRITE_TDM_WP       _IO(IO_MAGIC, 18)  /* Cmd for Reg write in Bar 2 */ 
#define SIG_SIGNAL_REGISTER         _IO(IO_MAGIC, 19)   
#define SIG_SIGNAL_UNREGISTER       _IO(IO_MAGIC, 20)
#define SIG_RESERVE_CORES           _IO(IO_MAGIC, 21)
#define SIG_RELEASE_CORES           _IO(IO_MAGIC, 22)
#define SIG_REQUEST_CORES           _IO(IO_MAGIC, 23)
#define SIG_QUERY_DRIVER_INFO       _IO(IO_MAGIC, 24)
#define SIG_QUERY_CHIP_STATUS       _IO(IO_MAGIC, 25)

#define SIG_DIRECT_CHIP_READ        _IO(IO_MAGIC, 26)
#define SIG_DIRECT_CHIP_WRITE       _IO(IO_MAGIC, 27)
#define SIG_HARD_RESET              _IO(IO_MAGIC, 28)

#define TI667x_DSP_INTA_SET         _IO(IO_MAGIC, 29)
#define TI667x_DSP_INTA_CLR         _IO(IO_MAGIC, 30)
#define TI667X_SET_DWNLD_DONE       _IO(IO_MAGIC, 31)
#define TI667x_SET_MASTER_PRIV_SET  _IO(IO_MAGIC, 32)
#define TI667X_PCIEEP_GET_PCI_INFO  _IO(IO_MAGIC, 33)
#define TI667X_ALLOC_HOST_BUFS      _IO(IO_MAGIC, 34)
#define TI667X_FREE_HOST_BUFS       _IO(IO_MAGIC, 35)
#define TI667X_ALLOC_C66X_ADDRS     _IO(IO_MAGIC, 36)
#define TI667X_FREE_C66X_ADDRS      _IO(IO_MAGIC, 37)
#define TI667X_MAP_HOST_TO_C66X     _IO(IO_MAGIC, 38)

#define SIG_GET_PCI_COMMON_PARENT_BUS_NUM  _IO(IO_MAGIC, 50)
 
/* Register offset definitions -- offsets to BAR0 in "host / TDM" FPGA logic.  All registers are 32-bit. */

#define REG_HPIC              0   /* HPI32 control/status */
#define REG_HPIA              1   /* HPI32 address */
#define REG_HPIDI             2   /* HPI32 data, auto-increment */
#define REG_HPID              3   /* HPI32 data, no increment */
#define REG_CTRLSTAT          4   /* FPGA logic control / status register */
#define REG_MODULE_SEL        5   /* Module select -- not used on PTMC modules.  Used for PC104 boards */
#define REG_DSP_CS            6   /* DSP chip select (or core select, depending on type of DSP / CPU module) */
#define REG_CHIP_SEL          REG_DSP_CS
#define REG_DSP_CS1           7   /* DSP / CPU core select (when DSP_CSn treated as core select and not chip select) */
#define REG_EXT1_SEL          REG_DSP_CS1
#define REG_DSP_CS2           8   /*  "  "  */
#define REG_EXT2_SEL          REG_DSP_CS2
#define REG_DSP_RESET         9
#define REG_RESET             REG_DSP_RESET
#define REG_BM_HOST_ADDR      10
#define REG_BM_XFER_CTRL      11  /* Bus-master control register */
#define REG_DMA_CTRL          12  /* DMA control register (DMA between dual-port mem and peripherals, including DSP/CPU farm (HPI) */
#define REG_DMA_DSP           REG_DMA_CTRL
#define REG_BM_FLAGS          13  /* Bus-master results */
#define REG_INTR              14  /* Pending interrupts */

#define REG_FLASH_ADDR        15  /* Flash address register */
#define REG_FLASH_DATA        16  /* Flash data */
#define REG_AGGR_ADDR         17  /* Aggregation logic address */
#define REG_AGGR_DATA         18  /* Aggregation logic data */
#define REG_PERIPH_CTRL1      19  /* Peripheral control:  Flash resets, PHY control bits, etc */
#define REG_PERIPH_CTRL2      20  /* Peripheral control 2 (expansion) */
#define REG_LED_FP_CTRL       21  /* PTMC front-panel LEDs */
#define REG_LED_BD_CTRL       22  /* PTMC module LEDs -- row of 16 used for BIST, error codes, user-defined, etc */
#define REG_LOGIC_REV_ID      23  /* logic revision and ID register (read-only) */
#define REG_UART_CTRL         24  /* PCI-to-UART interface control register */


/* Aggregation FPGA registers (these values written to REG_AGGR_ADDR before read/write REG_AGGR_DATA) */

#define REG_AGGR_STATUS           0
#define REG_AGGR_ROUTING_CTRL     1
#define REG_AGGR_TEST             2
#define REG_AGGR_LOGIC_REV_ID_LO  3
#define REG_AGGR_LOGIC_REV_ID_HI  4
#define REG_AGGR_PN4_MCBSP_CTRL   5


/*TDM / CTBus registers (see tdmlib.h for documentation) */

#define REG_TDM_BASE          32

#define REG_TDM_CTRLSTAT      32  /* TDM control/status register */

#define REG_TDM_CT_CLKSRC     33  /* Defines clock source for CTbus lines */

#define REG_TDM_DSP_CLKSRC0   34  /* Defines clock source for DSPs 0-5 */
#define REG_TDM_DSP_CLKSRC1   35  /* Defines clock source for DSPs 6-11 */

#define REG_TDM_CLK_FS_CTRL   36  /* Internal clock and framesync generation control */

#define REG_TDM_STREAM_A_TX   40  /* Each stream mapping register has 32-bit Rx and Tx section */
#define REG_TDM_STREAM_A_RX   41
#define REG_TDM_STREAM_B_TX   42
#define REG_TDM_STREAM_B_RX   43
#define REG_TDM_STREAM_C_TX   44
#define REG_TDM_STREAM_C_RX   45
#define REG_TDM_STREAM_D_TX   46
#define REG_TDM_STREAM_D_RX   47
#define REG_TDM_STREAM_E_TX   48
#define REG_TDM_STREAM_E_RX   49
#define REG_TDM_STREAM_F_TX   50
#define REG_TDM_STREAM_F_RX   51
#define REG_TDM_STREAM_G_TX   52
#define REG_TDM_STREAM_G_RX   53
#define REG_TDM_STREAM_H_TX   54
#define REG_TDM_STREAM_H_RX   55
#define REG_TDM_STREAM_I_TX   56
#define REG_TDM_STREAM_I_RX   57
#define REG_TDM_STREAM_J_TX   58
#define REG_TDM_STREAM_J_RX   59
#define REG_TDM_STREAM_K_TX   60
#define REG_TDM_STREAM_K_RX   61
#define REG_TDM_STREAM_L_TX   62
#define REG_TDM_STREAM_L_RX   63


/* Other register bit definitions follow... */

/* Bit definitions for reg_ctrlstat */

#define CTRLSTAT_PROC_RST     4         /* DSP/CPU reset control bit */
#define CTRLSTAT_HOST_LOCK    16        /* lock access to host (PCI); don't allow WinPath or DMA controller */

/* Bit definitions for reg_periph_ctrl */

#define FLASH_RST             1

/* logic revision / ID register format

    31-24       23-16       15-10      9-8       7-4      3-0
  REV_MAJOR   REV_MINOR   RESERVED   FPGA_ID   CONF_ID   BOARD_ID
    
  Examples:

  0x00130041  (Rev 1.30 host logic, 5561 PTMC module)
  0x00210131  (Rev 2.11 host/TDM logic, 641x PTMC module)
*/

#define REV_MAJOR             24   /* shift amounts to extract field */
#define REV_MINOR             16
#define FPGA_ID               8    /* 0 = host/TDM, 1 = Aggr */
#define CONF_ID               4    /* 0 = C54xx, 1 = C55xx, 2 = C671x, 3 = 641x, 4 = 5561 + WinPath, 5 = 5561 no WinPath */
#define BOARD_ID              0    /* board configuration, 0 = PC104, 1 = PTMC, 2-15 reserved */


/* Bit fields used for both bus master read/writes and hpi burst reads/writes
   Used when initiating a transfer via writing REG_BM_XFER_CTRL or REG_DMA_CTRL */

#define DMA_BUFFER0           0x00000000
#define DMA_BUFFER1           0x40000000
#define DMA_WRITE             0x00000000
#define DMA_READ              0x80000000
#define DMA_PUSH              0x20000000

/* Bit fields of REG_INTR */

#define INT_DMA_HOST          0x00000001  /* PCI/PCIe interrupt bit set by FPGA logic upon completion of a bus master transfer */
#define INT_DMA_DSP           0x00000002  /* interrupt bit set by FPGA logic upon completion of an HPI burst transfer */

#define HINT0                 0x00000100  /* Bits 15-8 are HINTs (Host interrupts) from individual devices */
#define HINT1                 0x00000200
#define HINT2                 0X00000400
#define HINT3                 0X00000800
#define HINT4                 0X00001000
#define HINT5                 0x00002000
#define HINT6                 0x00004000
#define HINT7                 0x00008000
#define HINTMASK              0x0000ff00

/* Supported card / CPU memory access modes */

#define MODE_DIRECT           0  /* Use single transfers directly to/from HPIA and HPID. Very slow, but the simplest way to transfer */
#define MODE_SLAVE            1  /* Permit burst HPI transfers into host logic buffer, but only read/write the buffer slave PCI transactions */
#define MODE_MASTER           2  /* Permit burst HPI and bus mastering PCI transfers */
#define MODE_MASTER_PUSH      3  /* Burst HPI, bus mastering and no delay between the two. Most complicated logic, best throughput. */
#define MODE_NOWAIT           4  /* Same as MODE_MASTER_PUSH except can use poll() */
#define MAX_MODE MODE_NOWAIT

/* SIGC667X */

#define TI667X_EP_L2SRAM_BASE         0x00800000     /* C6678 core0 L2 memory */
#define TI667X_EP_MSMCSRAM_BASE       0x0C000000     /* C6678 Multicore Shared Memory */
#define TI667X_EP_DDR3_BASE           0x80000000     /* C6678 DDR3 memory */
#define TI667X_EP_CHIP_CFG_BASE       0x02300000
#define TI667X_EP_CHIP_CFG2_BASE      0x01800000
#define TI667X_EP_DDR3_CTRL_BASE      0x21000000     /* C6678 DDR3 controller base address */
#define TI667X_EP_DDR3_PLL_BASE       0x02600000

#define TI667X_EP_PCIE_BASE           0x21800000     /* C6678 PCIE base address */
#define TI667X_EP_PID_OFFSET          0x0            /* Peripheral Version and ID Register */
#define TI667x_EP_RSTCMD_OFFSET       0x14           /* Reset Command Register */
#define TI667X_EP_ENDIAN_OFFSET       0x38           /* Endian Mode Register */
#define TI667X_EP_IRQ_SET_OFFSET      0x64           /* Endpoint Interrupt Request Set Register */
#define TI667X_EP_IRQ_CLR_OFFSET      0x68           /* Endpoint Interrupt Request Clear Register */
#define TI667X_EP_IRQ_STATUS_OFFSET   0x6C           /* Endpoint Interrupt Status Register */
#define TI667X_EP_INTA_SET_OFFSET     0x180          /* PCIE legacy interrupt A SET */
#define TI667X_EP_INTA_CLR_OFFSET     0x184          /* PCIE legacy interrupt A CLR */
#define TI667X_EP_INTA_ENABLE_SET     0x188          /* PCIE legacy interrupt enable A SET */
#define TI667X_EP_INTA_ENABLE_CLR     0x18C          /* PCIE legacy interrupt enable A CLR */
#define TI667X_EP_BOOTFLAG_OFFSET     0x0007FFFC     /* the last address in the respective local L2 */

#define TI667X_EP_L2SRAM_MAX_SIZE     0x00080000     /* L2 memory size is 512k bytes */
#define TI667X_EP_MSMCSRAM_MAX_SIZE   0x00400000     /* shared memory size is 4M bytes */
#if 0
#define TI667X_EP_DDR3_MAX_SIZE       0x40000000     /* DDR size is 1G bytes */
#else
#define TI667X_EP_DDR3_MAX_SIZE       0x7fffffff     /* DDR size is 2G bytes for A103 version of Advantech cards*/
#endif
#define TI667X_EP_CHIP_CFG_MAX_SIZE   0x00800000     /* Chip config space maximum size */
#define TI667X_EP_CHIP_CFG2_MAX_SIZE  0x00400000     /* Chip config space 2 maximum size */
#define TI667X_EP_DDR3_CTRL_MAX_SIZE  0x00800000     /* DDR3 control register area maximum size */
#define TI667X_EP_DDR3_PLL_MAX_SIZE   0x00400000
#define TI667X_EP_PCIE_MAX_SIZE       0x00800000

#define TI667X_EP_MAP_OFFSET_ALIGN    0x000000FF     /* Mapping alignment */
#define TI667X_EP_MAP_ALIGN           0xFFFFFF00     /* Mapping alignment */

#define TI667X_EP_L2MAP_OFFSET_ALIGN  0x000FFFFF     /* L2 Mapping alignment */
#define TI667X_EP_L2MAP_ALIGN         0xFFF00000     /* L2 Mapping alignment */

#define TI667X_SHARED_MEMORY_REGION_SIZE      0x400000
#define TI667X_PCIE_MAX_IO_BUFFERS            2 
#define TI667X_PCIE_OB_CONFIG_SPACE_BUFFERS   4
#define TI667X_PCIE_MAX_SHARED_MEM_BUFFERS    26
#define TI667X_OB_SIZE_1MB                    0
#define TI667X_OB_SIZE_2MB                    1
#define TI667X_OB_SIZE_4MB                    2
#define TI667X_OB_SIZE_8MB                    3

#define TI667X_DMA_TIMEOUT            100000

#define MAX_DRIVER_DEVICES            256            /* maximum devices currently handled by the Sig driver */
#define MAX_CORES_PER_DEVICE          8              /* current max cores per device, subject to change */

typedef struct {

   unsigned long   bar_start;
   unsigned long   bar_len;
   unsigned long   bar_flags;

} res_bar_info_t;

typedef struct {

   unsigned int    u_devid;       /* Device ID 0..N-1 (input-- set prior to ioctl call to specify which device) */
   unsigned char   bus_number;    /* bus number */
   unsigned char   bridge_pri;    /* number of primary bridge */
   unsigned short  vendor;
   unsigned short  device;
   unsigned short  subsystem_vendor;
   unsigned short  subsystem_device;
   unsigned int    cardclass;     /* 3 bytes: (base,sub,prog-if) */
   uint8_t         hdr_type;      /* PCI header type (`multi' flag masked out) */
   uint8_t         pin;           /* which interrupt pin this device uses */

   res_bar_info_t  bar_info[4];  /* I/O and memory regions + expansion ROMs */

} pci_dev_info_t;

typedef enum {

   SIG_RSV_DC = 1,
   SIG_RSV_QEMU = 2

} sig_reserve_handle_flags_t;

typedef struct {

   unsigned int nCores;
   sig_reserve_handle_flags_t flags;
   
} sig_reserve_handle_info_t;

typedef struct {

   unsigned int nSigC6678Cores_total;
   unsigned int nSigC6678Devices_total;
   unsigned int nSigC6678Cores_reserved;
   sig_reserve_handle_info_t nSigC6678Cores_reserved_handles[MAX_SIGC6678_DEVICES];

} sig_driver_info_t;

/* Currently only dynamic is supported  */
/* Persistent : Always get the same physical memory. This is useful
                during development, when host process exists and restarts
                and not necessary to reset and re-download TI CPU, especially
                if CPU is using Host memory as Global shared memory across 
                all CPUs */
#define DS_PERSISTENT_HOST_BUF 0
/* Dynamic : Application has to make free up calls to free memory when exiting 
*/
#define DS_DYNAMIC_HOST_BUF    1

/* Maximum number of buffers allocated per API call*/
#define MAX_CONTIG_BUF_PER_ALLOC 64

typedef struct  {

   uint64_t physAddr;            /* physical address ; also visible in the
                                    pci address space from root complex*/
   uint8_t *userAddr;            /* Host user space Virtual address */
   uint32_t length;              /* Length of host buffer */

} host_buf_desc_t;

/* Basic information about host buffer accessible by target CPU through PCIE */
typedef struct {

   uint64_t dmaAddr;          /* PCIe address */
   uint8_t *virtAddr;         /* Host Virtual address */
   uint32_t length;           /* Length of host buffer */
   
} host_buf_entry_t;

/* List of Buffers  */
typedef struct {

   unsigned int num_buffers;      /* Number of host buffers */
   unsigned int type;             /* memory type 0; Persistent; 1; Dynamic */
   host_buf_entry_t buf_info[MAX_CONTIG_BUF_PER_ALLOC];
   
} ioctl_host_contig_buf_info_t;

/* C66x address allocation/free info */
typedef struct {
   
   uint16_t num_contiguous_regions;    /* number of contiguous memory regions */
   uint32_t chip_start_addr;            /* C66x address for start of region */
   
} ioctl_c66x_addr_info_t;

/* info for mapping host mem to c66x addresses */
typedef struct {

   uint32_t dsp_start_addr;
   uint32_t num_of_bufs;
   host_buf_desc_t *buf_desc;
   
} ioctl_host_to_c66x_info_t;

/* struct for direct chip access */
typedef struct {
   
   uint32_t chip_id;
   uint32_t address;
   uint32_t length;
   void *buffer;

} ioctl_direct_chip_access;

#ifdef _VIRTIO_SIG_
typedef enum {
   
   /* driver APIs */
   SIG_MC_HW_READ,
   SIG_MC_HW_WRITE,
   SIG_MC_HW_OPEN,
   SIG_MC_HW_RELEASE,
   SIG_MC_HW_IOCTL,
   SIG_MC_HW_POLL

} virtio_sig_cmd_t;

typedef struct {
   
   uint32_t nCores;

} virtio_sig_config_t;
#endif

/******************************************************************************************
End of exported definitions, start internal definitions
*******************************************************************************************/


/* definitions within __KERNEL__ are visible only to drivers, not user space libs or programs.  __KENERL__ is set automatically by kbuild -- don't override this */
#ifdef __KERNEL__

#define CMD_STATUS         0x004
#define OB_SIZE            0x030
#define OB_OFFSET_INDEX(x) (0x200 + (0x08 * x))
#define OB_OFFSET_HI(x)    (0x204 + (0x08 * x))
#define IB_BAR(x)          (0x300 + (0x10 * x))
#define IB_START_LO(x)     (0x304 + (0x10 * x))
#define IB_START_HI(x)     (0x308 + (0x10 * x))
#define IB_OFFSET(x)       (0x30c + (0x10 * x))

/* Application command register values */
#define POSTED_WR_EN       BIT(3)
#define IB_XLAT_EN_VAL     BIT(2)
#define OB_XLAT_EN_VAL     BIT(1)

/* C6678 EDMA registers */
#define EDMA_TPCC0_BASE_ADDRESS     0x02700000
#define DCHMAP                      0x0100
#define DMAQNUM0                    0x0240  
#define ESR                         0x1010 
#define EESR                        0x1030                 
#define IESR                        0x1060
#define IPR                         0x1068 
#define ICR                         0x1070 
#define PARAM_0_OPT                 0x4000
#define PARAM_0_SRC                 0x4004
#define PARAM_0_A_B_CNT             0x4008
#define PARAM_0_DST                 0x400C
#define PARAM_0_SRC_DST_BIDX        0x4010
#define PARAM_0_LINK_BCNTRLD        0x4014
#define PARAM_0_SRC_DST_CIDX        0x4018
#define PARAM_0_CCNT                0x401C
#define PARAM_1_OPT                 0x4020
#define PARAM_1_SRC                 0x4024
#define PARAM_1_A_B_CNT             0x4028
#define PARAM_1_DST                 0x402C
#define PARAM_1_SRC_DST_BIDX        0x4030
#define PARAM_1_LINK_BCNTRLD        0x4034
#define PARAM_1_SRC_DST_CIDX        0x4038
#define PARAM_1_CCNT                0x403C
#define PARAM_2_OPT                 0x4040
#define PARAM_2_SRC                 0x4044
#define PARAM_2_A_B_CNT             0x4048
#define PARAM_2_DST                 0x404C
#define PARAM_2_SRC_DST_BIDX        0x4050
#define PARAM_2_LINK_BCNTRLD        0x4054
#define PARAM_2_SRC_DST_CIDX        0x4058
#define PARAM_2_CCNT                0x405C

#define PCIE_DATA      0x60000000 
/* Payload size in bytes over PCIE link. PCIe module supports outbound payload size of 128 bytes and inbound payload size of 256 bytes */
#define PCIE_TRANSFER_SIZE   0x80               

#define DMA_QUEUE_RD    0
#define DMA_QUEUE_WR    1
/* let c66x code use 0-7 */
#if 0
#define DMA_CHAN_RD     2
#define DMA_CHAN_WR     3
#endif
#define DMA_CHAN_RD     8
#define DMA_CHAN_WR     9

/********************************************************************************************/
/* Function Prototypes */
/********************************************************************************************/
/* Used to print debug messages to the active tty */
static void print_string(char *);

/* called when open(...) is used to open the device */
static int     sig_mc_hw_open         (struct inode*, struct file*);
#ifdef _SRIO_
static int     sig_mc_hw_srio_open    (struct inode*, struct file*);
#endif
/* called with the device is closed */
static int     sig_mc_hw_release      (struct inode*, struct file*);
#ifdef _SRIO_
static int     sig_mc_hw_srio_release (struct inode*, struct file*);
#endif
/* for read() or pread() */
static ssize_t sig_mc_hw_read         (struct file*,  char*, size_t, loff_t*);
#ifdef _SRIO_
static ssize_t sig_mc_hw_srio_read    (struct file*,  char*, size_t, loff_t*);
#endif
/* write or pwrite() */
static ssize_t sig_mc_hw_write        (struct file*,  const char*, size_t, loff_t*);
#ifdef _SRIO_
static ssize_t sig_mc_hw_srio_write   (struct file*,  const char*, size_t, loff_t*);
#endif
/* ioctl() */
static long    sig_mc_hw_ioctl        (struct file*, unsigned int, unsigned long);
#ifdef _SRIO_
static long    sig_mc_hw_srio_ioctl   (struct file*, unsigned int, unsigned long);
#endif
/* poll() */
static unsigned int sig_mc_hw_poll    (struct file*, poll_table*);

static int sig_mc_hw_mmap             (struct file*, struct vm_area_struct*);

#ifdef _VIRTIO_SIG_
/* Use virtio probe function with virtio driver */
static int virtsig_probe(struct virtio_device *vdev);
#else
/* After registering the pci side of the driver, the kernel will call pci_probe */
/* with device candadits that the driver can choose to support */
static int     pci_probe(struct pci_dev*, const struct pci_device_id*);
#endif

/* The interrupt handler for the devices */
/* Translates pci interrupts into driver events for read/write to act upon */

/* static void IntrHandler(int, void*, struct pt_regs*); */

/********************************************************************************************/

/**************************************************************************/
/* The Driver Structure. */
/* There will be one structure each physical device handled by the driver */
/**************************************************************************/

typedef struct /*_host_buf_info_t*/ {

/*   u32 pcieAddr; */
   u64 pcieAddr;
   u8 *virtAddr;
   unsigned int length;

} host_buf_info_t;

/* List of Buffers */
typedef struct {

   unsigned int num_buffers;      /* Number of host buffers */
   unsigned int type;             /* memory type 0; Persistent; 1; Dynamic */
   host_buf_entry_t *buf_info;
   
} host_contig_buf_info_t;

#define OBREG_FLAG_ALLOCATED	 0x1
typedef struct {

   uint32_t flags;
   uint16_t ob_reg_number;
} pciedrv_obreg_alloc_inst_t;

typedef struct {

   struct pci_dev* pPciDevice;
   
   /* SigC6415 Specifics */
   u32* dma_buffer;        /* kernal space pointer into the dma buffer */
   u32* read_buffer;
   u32* write_buffer;
   dma_addr_t bus_addr;    /* pci bus address of the dma buffer */
   dma_addr_t rbuf_bus_addr;
   dma_addr_t wbuf_bus_addr;
   wait_queue_head_t HpiMasterQueue;

   /* TI667x Specifics */
   uintptr_t *BAR3;        /* Local pointer to BAR 3 */
   uintptr_t *BAR4;        /* Local pointer to BAR 4 */
   host_buf_info_t pci_io_buf_info[TI667X_PCIE_MAX_IO_BUFFERS];
   
   /* General */
   uintptr_t* BAR0;        /* Local pointer to BAR 0 */
   uintptr_t* BAR1;        /* Local pointer to BAR 1 */
   uintptr_t* BAR2;        /* Local pointer to BAR 2 */

   wait_queue_head_t BusMasterQueue;
   u32 ReadMode;           /* Which transfer mode to use while reading */
   u32 WriteMode;          /* Which transfer mode to use while writing */
   u32 ChunkSize;          /* Number of words to transfer per hpi burst transaction */
   u32 Event;              /* Indicates local events */
   struct semaphore sem;   /* Used to lock read/write/ioctl to protect reentrent code */
   u32 Busy;               /* 1 = read, 2 = write */
   u32 ReadReady;          /* Set read DMA completes */
   u32 XferAddr;           /* read addr */
   u32 XferSize;           /* read size (in bytes) */
   wait_queue_head_t wq;
   
   u32 device_id;
   atomic_t* active_devices_ptr;
   
   u8 bBAR_mode;           /* 0 = 32-bit BAR mode, 1 = 64-bit.  Per device value.  JHB Feb 2016 */

} sig_mc_hw_t;

#ifdef _VIRTIO_SIG_
typedef struct {
   
   struct virtio_device *pVirtioDevice;
   struct virtqueue **vq;
   wait_queue_head_t waitqueue;
   
} virtio_sig_t;
#endif

/* struct to represent a list of chips */
typedef struct {
   
   unsigned int num_chips;
   unsigned int *chips;
   unsigned char *cores;
   
} chip_list_t;

typedef struct {

   unsigned int num_chips_available;
   unsigned int *chip_list_status;
   chip_list_t chip_list;
   sig_reserve_handle_flags_t flags;

} reserve_pool_t;

typedef struct {
   
   /* HPI emulation vars associated with this specific handled */
   unsigned int HPI_addr;
   unsigned int HPI_config;
   
   /* Cores associated with this specific file handle */
   /* unsigned int num_cores; */
   unsigned long long core_select;
   
   /* Used for MODE_NOWAIT read/writes to track when to wake user space app */
   atomic_t active_devices;
   
   /* core list of cores requested by the application using this file handle */
   chip_list_t requested_chips;
   unsigned char chips_assigned;
   unsigned int reserve_pool_index;
   
   /* pointer to contiguous memory buffers */
   host_contig_buf_info_t *dyn_host_buf_info;

} sig_file_handle_info_t;

/********************************************************************************************/

/********************************************************************************************/
/* Driver Specific defines */
/********************************************************************************************/
/* Device name in /proc/devices and /proc/interrupts */
#define DRIVER_NAME "sig_mc_hw"
#define SIGC5561 "sigc5561_%u"
#define SIGC6415 "sigc641x_%u"
#define SIGC6678 "sigc667x_%u"
#define ADV8901  "adv8901_%u"
   
/* Device base minor numbers */
#define SIG_MC_HW_MINOR 0x00
#define SIGC6415_MINOR  0x10
#define SIGC5561_MINOR  0x20
#define SIGC6678_MINOR  0x30
#define ADV8901_MINOR   0x00  /* note -- ADV8901 has SRIO interface, no PCIe, and uses a separate fops struct */

/* Set the maximum number of processes that can run concurrently */
#define MAX_TASKS 32

/* Bit filed used inside the Event member of the SigC6415 device structure in the driver */
/* Used to notify the driver, via the interrupt handler, that an event has occured */
#define EVENT_HPI 1
#define EVENT_PCI 2

/* Identify the vendor id and device id of the pci devices the driver will support */
#define SIGC6415_PCI_VENDOR_ID      0x1176
#define SIGC6415_PCI_DEVICE_ID      0x0B01

#define TI667X_PCI_VENDOR_ID        0x104c
#define TI667X_PCI_DEVICE_ID        0xb005  /* C6678 */

#define SUCCESS 0

/********************************************************************************************/

#endif  /* #ifdef __KERNEL__ */

#endif  /* #ifndef __SIGCX_DRIVER__ */
