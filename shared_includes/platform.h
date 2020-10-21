/* ========================================================================== */
/**
 *  @file   platform.h
 *
 *  @brief  The Platform Library is a thin utility
 *  layer on top of  CSL and other board utilities. It provides uniform APIs
 *  for all supported platforms. It aims to assist user to quickly write portable
 *  applications for its supported platforms by hiding board level details
 *  from the user.
 *
 *  ============================================================================
 */
/* --COPYRIGHT--,BSD
 * Copyright (c) 2010-2011, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/

/* added DDR3 register definitions, JHB Feb 2016 */

#ifndef PLATFORM_UTILS_H_
#define PLATFORM_UTILS_H_

/**
 * @mainpage Platform Utility APIs
 *  Defines a set of APIs for accessing and working with the various platform peripherals.
 *
 */


/** @defgroup  Platform_standard_data_types  Standard Data Types */
/*@{*/
/**
 *  @brief 	Platform Library uses basic C99 data types. The basic types used are
 *  		uint32_t, int32_t, uint8_t, int8_t, uint16_t and int16_t. The standard
 *  		C char is used for strings. Complex types (or typdefs) if used are defined
 *  		within this header file.
 */
#include <stdint.h>
#define EXAMPLE_VARIABLE uint32_t
/*@}*/  /* defgroup */

/** @defgroup  Platform_memory_section  Linker Memory Sections */
/*@{*/
/**
  * @brief  Memory Sections. You need to place these in your application linker map.
  *
  *			Section name: PLATFORM_LIB_SECTION
  *
  *			All of the static information used within the platform library is stored
  *			within this section.
  */
 #define PLATFORM_LIB_SECTION "This section defines the memory section for the platform library"
/*@}*/  /* defgroup */

/** @defgroup  Platform_cache Platform Cache */
/*@{*/
/**
  * @brief  The following definitions are for handling cache alignment on the platform.
  *
  * 		MAX_CACHE_LINE must be set to the cache line size of the platform.
  *
  * 		When allocating memory that must be cache aligned, it must be a multiple of
  * 		the cache line size. Use platform_roundup to get the appropriate size.
  *
  * 		As an example to allocate a cache aligned block of memory you would do
  * 		something like:
  *
  * 			buffer_len_aligned = platform_roundup (buffer_len, MAX_CACHE_LINE)
  * 			Malloc (buffer_len_aligned)
  *
  */
/* w should be power of 2 */
#define PLATFORM_CACHE_LINE_SIZE (128)
#define platform_roundup(n,w) (((n) + (w) - 1) & ~((w) - 1))

/**
 *   @n@b Convert_CoreLocal2GlobalAddr
 *
 *   @b Description
 *   @n This API converts a core local L2 address to a global L2 address.
 *
 *   @param[in]  addr  L2 address to be converted to global.
 *
 *   @return    uint32_t >0  Global L2 address
 *
 */
uint32_t Convert_CoreLocal2GlobalAddr (uint32_t  addr);


/*@}*/  /* defgroup */

/** @defgroup  Compilation_Flags Compilation Flags */
/*@{*/
/***
  * @brief  Flags for compiling the library.
  *
  * 		The file paltform_internal.h has compilation flags that can be set.
  */
/*@}*/  /* defgroup */

/** @defgroup  Platform_common  Common */
/*@{*/

/**
 * Error codes used by Platform functions. Negative values are errors,
 * while positive values indicate success.
 */

#define Platform_STATUS        int32_t /** Platform API return type */

#define Platform_EINVALID     -3   /**< Error code for invalid parameters */
#define Platform_EUNSUPPORTED -2   /**< Error code for unsupported feature */
#define Platform_EFAIL        -1   /**< General failure code */
#define Platform_EOK           0   /**< General success code */

/* Set the endianess for the platform */
#define PLATFORM_LE 1
#define PLATFORM_BE 0

#define PLATFORM_MAX_EMAC_PORT_NUM       2   /**< Maximum number of EMAC ports */

/**
 *  @brief This structure contains multicore processor information, e.g. # of the core, processor name, etc.
 */
typedef struct {
    int32_t core_count;
    /**<Number of cores*/
    char name[32];
    /**<Name of processor (eg: TMS320C6472)*/
    uint16_t id;
    /**<CPU ID of the Device (eg. Ch -> C64x CPU, 10h -> C64x+ CPU).*/
    uint16_t revision_id;
    /**<CPU Revision ID*/
    uint16_t megamodule_revision_major;
    /**<Megamodule Revision ID Major*/
    uint16_t megamodule_revision_minor;
    /**<Megamodule Revision ID Minor*/
    uint16_t silicon_revision_major;
    /**<Silicon Revision ID Major*/
    uint16_t silicon_revision_minor;
    /**<Silicon Revision ID Minor*/
    uint8_t endian;
    /**<Endian: {PLATFORM_LE | PLATFORM_BE}*/
} CPU_info;

/**
 *  @brief This structure contains information about the EMAC, e.g. # of EMAC port, MAC address for the port, etc.
 */
typedef struct {
    int32_t port_count;
    /**<Number of EMAC ports*/
    uint8_t efuse_mac_address[6];
    /**<EFUSE EMAC address */ /* August 15, 2011 - this field is deprecated, MAC address is now defined in the new data structure PLATFORM_EMAC_EXT_info */
    uint8_t eeprom_mac_address[6];
    /**<EEPROM EMAC address */ /* August 15, 2011 - this field is deprecated, MAC address is now defined in the new data structure PLATFORM_EMAC_EXT_info */
} EMAC_info;


/**
 * @brief Indicates the EMAC port mode
 *
 */
typedef enum {
    PLATFORM_EMAC_PORT_MODE_NONE,
    /**<EMAC port not used */
    PLATFORM_EMAC_PORT_MODE_PHY,
    /**<EMAC port connected to a PHY */
    PLATFORM_EMAC_PORT_MODE_AMC,
    /**<EMAC port connected to the backplane AMC chassis */
    PLATFORM_EMAC_PORT_MODE_MAX
    /**<End of port mode */
} PLATFORM_EMAC_PORT_MODE;


/**
 *  @brief This structure contains extended information about the EMAC, e.g. port #, port mode, port MAC addess, etc.
 */
typedef struct {
    uint32_t                        port_num;
    /**<Port number of the EMAC port */
    PLATFORM_EMAC_PORT_MODE         mode;
    /**<Mode of the EMAC port */
    uint8_t                         mac_address[6];
    /**<MAC address of the EMAC port */
} PLATFORM_EMAC_EXT_info;


/** @brief LED Classes */

typedef enum {
    PLATFORM_USER_LED_CLASS,
    /** <USER LED Group */
    PLATFORM_SYSTEM_LED_CLASS,
    /** <SYSTEM LED Group */
    PLATFORM_END_LED_CLASS
    /** END OF LED Groups */
} LED_CLASS_E;

/**
 *  @brief This structure contains information about LED on the platform
 */
typedef struct {
    int32_t count;
    /**<Number of LEDs*/
} LED_info;

/**
 * @brief Define how platform_write should behave.
 *    These write types can be set in the init structure
 */

typedef enum {
    PLATFORM_WRITE_UART,
    /** <Write to the UART */
    PLATFORM_WRITE_PRINTF,
    /** <printf mapped output -- CCS console */
    PLATFORM_WRITE_ALL
    /** <write all - default configuration */
} WRITE_info;

/**
 *  @brief This structure contains board specific information, e.g. cpu info, board Rev., LED info, etc.
 */
typedef struct {
    char version[16];
    /**<Platform library version */
    CPU_info cpu;
    /**<CPU information */
    char board_name[32];
    /**<Name of the board */
    char serial_nbr[16];
    /**<Serial number for the unit as read from the I2C */
    uint16_t board_rev;
    /**<Revision number of the board, as read from the H/W*/
	uint32_t frequency;
    /**<CPU frequency (MHz)*/
    /**<Peripheral information */
    EMAC_info emac;
    /**<EMAC information*/
    LED_info led[PLATFORM_END_LED_CLASS];
    /**<LED information*/
} platform_info;


/**
 *  @brief      Get platform information.
 *
 *
 *  @param[out] p_info  This structure will have platform information on return
 *
 */

void platform_get_info(platform_info * p_info);

/**
 *  @brief This structure contains peripherals to be initialized. It provides for basic board initialization.
 *			Flash and Character device are intiialized and controlled when they are opened.
 *
 *  @remark
 *     The init flags are set when platform_init() API is called by the application during
 *     the board initialization, by default all the flags are set to 1.
 */
typedef struct {
    uint8_t pll;
    /**<1: initialize PLL */
    uint8_t ddr;
    /**<1: initialize ddr */
    uint8_t tcsl;
    /**<1: initialize Time Stamp Counter (Low) Register */ /* June 2, 2011 - This flag is now deprecated and the TCSL is always initialized */
    uint8_t phy;
    /**<1: initialize PHY and its dependent components */
    uint8_t ecc;
    /**<1: initialize memory ECC checks. If 0, they are not disabled but the default power on state is disabled. */
} platform_init_flags;


/**
 *  @brief This structure contains initialization parameters
 */

typedef struct {
    uint32_t pllm;
    /**<Platform pll multiplier (0 to set the default value)*/
    uint32_t plld;
    /**<Platform pll divider (0 to set the default value)*/
    uint32_t prediv;
    /**<Platform pll predivider (0 to set the default value)*/
    uint32_t postdiv;
    /**<Platform pll postdivider (0 to set the default value)*/
    uint16_t mastercore;
    /** Designates this core as the Master. Default is Core 0 */
 } platform_init_config;

/**
 *  @brief     Plarform initialization
 *
 *  @param[in] p_flags  This structure will have init enable flags for peripherals
 *
 *  @param[in] p_config  This structure will have init configuration parameters
 *
 *  @retval    Platform_EOK on Success
 *
 *  @remark    This function can be called multiple times to init various peripherals,
 *             normally this API is called first when board is initialized by the application
 *
 */

Platform_STATUS platform_init(platform_init_flags * p_flags, platform_init_config * p_config);


/**
 *  @brief     Test external (DDR) memory region
 *
 *  @param[in] start_address  DDR Address to start at
 *
 *  @param[in] end_address    DDR Address to end at
 *
 *  @retval    Platform_EOK on Success
 *
 *  @remark    If the test fails, platform_errno will be set to the DDR address the test
 *			   failed at.
 */

Platform_STATUS platform_external_memory_test(uint32_t start_address, uint32_t end_address);

/**
 *  @brief     Test external (DDR) memory region
 *
 *  @param[in] id  Core to run th einternal memory test on
 * *
 *  @retval    Platform_EOK on Success
 *
 *  @remark    If the test fails, platform_errno will be set to the DDR address the test
 *			   failed at.
 */

Platform_STATUS platform_internal_memory_test(uint32_t id);



/**
 *  @brief     Plarform get core_id
 *
 *  @retval    Returns current core ID
 *
 */

uint32_t platform_get_coreid(void);

/**
 *  @brief     Platform get DIP switch state
 *
 *  @param[in] id ID of the switch
 *
 *  @retval    1 if ON and 0 if OFF
 *
 */

uint32_t platform_get_switch_state(uint32_t id);

/*@}*/  /* defgroup */

/** @defgroup    EMAC_PHY_support_functions  EMAC */
/*@{*/

/**
 * @brief MAC address type
 *
 */
typedef enum {
    /** MAC address in EFUSE */
    PLATFORM_MAC_TYPE_EFUSE,
    /** MAC address in EEPROM */
    PLATFORM_MAC_TYPE_EEPROM
} PLATFORM_MAC_TYPE;

/**
 *  @brief     Plarform get MAC address from EFUSE
 *
 *             August 15, 2011 - this API is deprecated, application needs to call
 *             the new API platform_get_emac_info() to get the MAC address of the port
 *
 *  @param[in] type  MAC address storage type
 *
 *  @param[out] mac_address  MAC address assigned to the core
 *
 *  @retval    Platform_EOK on Success
 *
 */

Platform_STATUS platform_get_macaddr(PLATFORM_MAC_TYPE type, uint8_t * mac_address);

/**
 *  @brief     Plarform get information of an EMAC port
 *
 *  @param[in] port_num  port number
 *
 *  @param[out] emac_info  EMAC port information
 *
 *  @retval    Platform_EOK on Success
 *
 */

Platform_STATUS platform_get_emac_info(uint32_t port_num, PLATFORM_EMAC_EXT_info * emac_info);

/**
 *  @brief     Get PHY address for a port number
 *
 *             Please note that this function is a place holder for C64x devices
 *             and is not used for KeyStone devices.
 *
 *  @param[in] port_num  port number
 *
 *  @retval    event id or -1 on failure
 *
 */

int32_t platform_get_phy_addr(uint32_t port_num);

/**
 *  @brief     Platform EMAC/PHY link status
 *
 *  @param[in] port_num  port number
 *
 *  @retval    0 on success
 *
 *  @remark    This is ONLY supported for on chip PHY
 *
 */

Platform_STATUS platform_phy_link_status(uint32_t port_num);

/*@}*/  /* defgroup */

/** @defgroup    Memory_device_support_functions  Memory Devices (e.g. Flash) */
/*@{*/

/**
 * @brief Devices
 *         The platform library provides a common interface for reading and writing
 *         flash and serial memory devices on the platform.
 *
 *         To work with a device you must first open it, perform a read or write, and
 *         then close it.
 *
 *         In addition to the basic operations of read and write, certain types of devices, like flash
 *         may support other extended operations.
 *
 * Devices
 *
 *  Types: 	NOR
 *			NAND
 *			EEPROM
 *
 * Operations:
 *			Open
 *			Close
 *			Read
 *			Write
 *
 *	Extended Operations:
 *			Erase			Supported by NOR and NAND devices
 *			ReadSpare		Supported by NAND devices
 *			WriteSpare		Supported by NAND devices
 *			MarkBlockBad	Supported by NAND devices
 *
 *
 *  Nand Device Notes:
 *
 *  Writes
 *
 *  When writing the NAND device you may either a) Write one page at a time or
 *  b) write a block of data that is larger than a single page. All writes to the
 *  NAND flash must be aligned to start on a page boundary.
 *
 *  When using platform_device_write() if you write a single page (done by
 *  setting the length of the write to the page size) then your application will need to
 *  take care of erasing and preserving the block the page is in.
 *
 *  If you write more than a page, the platform_device_write call will handle
 *  erasing the block the page is in and will also preserve the contents of other
 *  pages within the block that are not being written to. The algorithm used is as
 *  follows:
 *
 * 	   While we have data to write do
 *       skip block if bad (and keep skipping until the next good block is found)
 *       read the block    (page level)
 *       erase the block   (block level)
 *       write the block   (page level)
 *
 * 	Reads
 *
 *  When using platform_device_read on a NAND device it will currently only read a
 *  single page at a time.
 *
 *
 *  Nor Device Notes
 *
 *  Writes
 *
 *  When working the NOR device you may either write a block of data that is larger than
 *  a single page. All writes to the flash must be aligned to start on a page boundary.
 *
 *  When using platform_device_write() if you write a single page (done by
 *  setting the length of the write to the page size) then your application will need to
 *  take care of erasing and preserving the block the page is in.
 *
 *  If you write more than a page, the platform_device_write call will handle
 *  erasing the block the page is in and will also preserve the contents of other
 *  pages within the block that are not being written to. The algorithm used is as
 *  follows:
 *
 * 	   While we have data to write do
 *       skip block if bad (and keep skipping until the next good block is found)
 *       read the block    (page level)
 *       erase the block   (block level)
 *       write the block   (page level)
 *
 *  Reads
 *
 *  When using platform_device_read on a NOR device, you may read as much as you like.
 */

/**
 * @brief 	Device Identifiers. These are used in the Open call to allow access to a specific
 * 			memory device.
 *
 */
#define    PLATFORM_DEVID_NAND512R3A2D      0x2036		/**< NAND Flash */
#define    PLATFORM_DEVID_NORN25Q128        0xBB18		/**< NOR Flash */
#define    PLATFORM_DEVID_EEPROM50   		0x50		/**< EEPROM @ slave address 0x50  */
#define    PLATFORM_DEVID_EEPROM51   		0x51		/**< EEPROM @ slave address 0x51  */

/**
 * @brief Indicates the type of device
 *
 */
typedef enum {
    PLATFORM_DEVICE_NAND,
    /**<NAND Flash*/
    PLATFORM_DEVICE_NOR,
    /**<NOR Flash*/
    PLATFORM_DEVICE_EEPROM,
    /**<NOR Flash*/
    PLATFORM_DEVICE_MAX
    /**<End of devices*/
} PLATFORM_DEVICE_TYPE;

/**
 *  @brief 	This type defines the opaque handle returned to a device that is opened.
 *  		The handle must be used in all subsequent operations.
 *
 */
typedef uint32_t PLATFORM_DEVHANDLE;

/**
 *  @brief This structure contains information about the flash device on the platform
 *
 *			The bblist points to an array of bytes where each position represents a
 *			block on the device. If the block is good it is marked as 0xFF. If the block
 *			is bad, it is marked as 0x00. For devices that do not support a bad block list this
 *			value will be NULL. The number of blocks in the bblist is determined by the block_count field.
 */
typedef struct {
    int32_t manufacturer_id;		/**<manufacturer ID*/
    int32_t device_id;				/**<Manufacturers device ID*/
    PLATFORM_DEVICE_TYPE  type;		/**<Type of device */
    int32_t width;					/**<Width in bits*/
    int32_t block_count;			/**<Total blocks. First block starts at 0. */
    int32_t page_count;				/**<Page count per block*/
    int32_t page_size;				/**<Number of bytes in a page including spare area*/
    int32_t spare_size;				/**<Spare area size in bytes*/
    PLATFORM_DEVHANDLE handle;		/**<Handle to the block device as returned by Open. Handle is Opaque, do not interpret or modify */
    int32_t	bboffset;				/**<Offset into spare area to check for a bad block */
	uint32_t column;				/**<Column for a NAND device */
	uint32_t flags;					/**<Flags is a copy of the flags that were used to open the device */
	void	*internal;				/**<Do not use. Used internally by the platform library */
    uint8_t *bblist;				/** <Bad Block list or NULL if device does not support one  */
} PLATFORM_DEVICE_info;

/**
 *  @brief       Opens a device for use
 *
 *  @param[in]   deviceid		Device to open
 *
 *	@param[in]	 flags			Various flags
 *
 *  @retval      NULL or a pointer to the File Handle if successful.
 *
 *  @remark
 *               On success a handle is returned in p_devinfo which should be used in
 *				 all subsequent calls. As of now, the devices are not virtualized and only
 *				 one open may exist at a time for a particular device.
 *
 *				 If NULL is returned paltform_errno should be set to indicate why the
 *				 open was un-successful.
 *
 * 				 Flag Usage: (Currently none defined, use 0)
 */

PLATFORM_DEVICE_info *platform_device_open(uint32_t deviceid, uint32_t flags);

/**
 *  @brief       Closes the device
 *
 *  @param[in]   handle  Handle to the device as returned in the open call.
 *
 *  @retval      Platform_EOK on Success
 *
 */

Platform_STATUS platform_device_close (PLATFORM_DEVHANDLE handle);


/**
 *  @brief      Write the data to the device
 *
 *  @param[in]  handle  Handle to the device as returned by open
 *
 *  @param[in]  offset 		Offset to start writing the data at.
 *
 *  @param[in] 	buf          Pointer to  data to write
 *
 *  @param[in] 	len          Length of the data pointed to by buf
 *
 *  @retval     Platform_EOK on Success
 *
 *  @remark		For NAND devices use the platform_blocknpage_to_offset call.
 *
 *
 */
Platform_STATUS platform_device_write(PLATFORM_DEVHANDLE 	handle,
									 uint32_t 	offset,
                                     uint8_t 	*buf,
                                     uint32_t	len);

/**
 *  @brief      Convert the block and page number to offset
 *
 *  @param[in]  handle  Handle to the device as returned by open
 *
 *  @param[in]  offset 		Offset to start writing the data at.
 *
 *  @param[in] 	block       Block number
 *
 *  @param[in] 	page        Page number
 *
 *  @retval     Platform_EOK on Success
 *
 *
 */
Platform_STATUS platform_blocknpage_to_offset(PLATFORM_DEVHANDLE 	handle,
									 uint32_t 	*offset,
                                     uint32_t 	block,
                                     uint32_t	page);


/**
 *  @brief      Convert the offset to block and page number
 *
 *  @param[in]  handle  Handle to the device as returned by open
 *
 *  @param[in]  offset 		Offset to start writing the data at.
 *
 *  @param[in] 	block       Pointer to the block number
 *
 *  @param[in] 	page        Pointer to the Page number
 *
 *  @retval     Platform_EOK on Success
 *
 *
 */
Platform_STATUS platform_offset_to_blocknpage(PLATFORM_DEVHANDLE handle,
									 uint32_t 	offset,
                                     uint32_t 	*block,
                                     uint32_t	*page);

/**
 *  @brief       Reads a page from the device
 *
 *  @param[in]   handle  Flash device handle from the open
 *
 *  @param[in]   offset Offset to start the read from
 *
 *  @param[in]   buf	Pointer to a buffer to read the data into
 *
 *  @param[in] 	len     Amount of data to read
 *
 *  @retval      Platform_EOK on Success
 *
 *  @remark      The buffer size should be page_size + spare_size
 *               The application should not write into the spare area
 *
 *               For NAND devices use the platform_offset_to_blocknpage call.
 *
 *  errno        This routine may set platform_errno to
 *               the following values on an error (see below for explanation):
 *               PLATFORM_ERRNO_ECC_FAIL
 */


Platform_STATUS platform_device_read(PLATFORM_DEVHANDLE 	handle,
									 uint32_t 	offset,
                                     uint8_t 	*buf,
                                     uint32_t	len);

/**
 *  @brief       Reads spare data from the flash device
 *
 *  @param[in]   handle  Flash device handle from the open
 *
 *  @param[in]   block_number Block ID to read from
 *
 *  @param[in]   page_number  Page to read the spare area from
 *
 *  @param[in]   buf          Pointer to message data
 *
 *  @retval      Platform_EOK on Success
 *
 *  @remark      The buffer size should be spare_size.
 */

Platform_STATUS platform_device_read_spare_data(PLATFORM_DEVHANDLE handle,
												uint32_t block_number,
												uint32_t page_number,
												uint8_t *buf);


/**
 *  @brief       Marks the block bad
 *
 *  @param[in]   handle  Handle from the open
 *
 *  @param[in]   block_number Block to write the spare area for
 *
 *
 *  @retval      Platform_EOK on Success
 *
 *  @remark      This API can be specifically used to mark a block to be bad 
 *               when there is read error due to the ECC failure. 
 *               The bad block mark byte is indexed by the bboffset. The application should
 *               only overwirte the bad block mark byte in the spare area
 *               data when marking a block bad.
 *
 */

Platform_STATUS platform_device_mark_block_bad (PLATFORM_DEVHANDLE handle,
                                                uint32_t block_number);

/**
 *  @brief       Writes spare data to the flash device
 *
 *  @param[in]   handle  Handle from the open
 *
 *  @param[in]   block_number Block to write the spare area for
 *
 *  @param[in]   page_number  Page to write the spare area for
 *
 *  @param[in]   buf          Pointer to spare area data to write
 *
 *  @retval      Platform_EOK on Success
 *
 *  @remark      This API can be used to mark a block to be bad when there
 *               is read error due to the ECC failure. The bad block mark
 *               byte is indexed by the bboffset. The application should
 *               only overwirte the bad block mark byte in the spare area
 *               data when marking a block bad.
 *
 *               The buffer size should be spare_size. This function
 *				 should ONLY be used when you know what you are doing
 *				 and have a specific purpose in mind. Incorrectly
 *				 changing the spare area of a block may lead to
 *				 unpredictable results or render it not useable.
 */

Platform_STATUS platform_device_write_spare_data(PLATFORM_DEVHANDLE handle,
                                                uint32_t block_number,
												uint32_t page_number,
												uint8_t *buf);

/**
 *  @brief       erase a block on the flash block
 *
 *  @param[in]   handle  Flash device handle from the open
 *
 *  @param[in]   block_number Block ID to erase
 *
 *  @retval      Platform_EOK on Success
 *
 */

Platform_STATUS platform_device_erase_block(PLATFORM_DEVHANDLE handle,
                                            uint32_t block_number);


/*@}*/  /* defgroup */

/** @defgroup    UART_functions  UART */
/*@{*/


/**
 *  @brief      Initialize the UART.
 *
 *  @retval     Platform_EOK on Success
 *
 *  @remark     This routine must be called before you read and write to the UART.
 *  			The default baudrate of 115200 will be set. It can be changed
 *  			by calling platform_uart_set_baudrate.
 */

Platform_STATUS platform_uart_init(void);


/**
 *  @brief      Set the baud rate for the UART
 *
 *  @param[in]  baudrate Baudrate to use: (2400, 4800, 9600, 19200, 38400, 57600, 115200)
 *
 *  @retval     Platform_EOK on Success
 *
 */

Platform_STATUS platform_uart_set_baudrate(uint32_t baudrate);

/**
 *  @brief      Read a byte from UART
 *
 *  @param[in]  buf  Pointer to message data
 *
 *  @param[in]  delay Wait time (in micro-seconds)
 *                    for input FIFO to be non-empty.
 *                    0 => Wait for ever.
 *
 *  @retval     Platform_EOK on Success
 *
 */

Platform_STATUS platform_uart_read(uint8_t *buf, uint32_t delay);

/**
 *  @brief      Write a character to the UART
 *
 *  @param[in]  chr 	character to write
 *
 *
 *  @retval     Platform_EOK on Success
 *
 */

Platform_STATUS platform_uart_write(uint8_t chr);

/**
 *  @brief       Platform Write
 *
 *  @param[in]   *fmt    printf style vararg
 *
 *  @retval      Nothing (but platform_errno may get set)
 *
 *  @remark      This routine will output the printf style string
 *               to one or both of the UART and/or through a printf
 *               call (in CCS this is mapped to the console window).
 *               By default, both are written. This can be controlled
 *               by setting write_type in the paltform_init structure.
 *               By default, both the UART and printf outputs are used.
 *
 *				 The largest string size you can write is 80 characters.
 *				 Some checking is performed (on the fmt string only) to be
 *				 sure it is under that length. Note that expansion could
 *				 set the string higher and therefore corrupt memory.
 *
 *               User platform_write_configure to control where the
 *               output appears. The call retturns the previous setting.
 *
 *				 This call is not intended to be used for serious debugging.
 *				 Its purpose is light duty writing of messages. It should not
 *               be called from an interrupt context as it uses printf when 
 *				 writing to the console. The following wiki articles are good
 *               write ups on printf vs. system_printf vs. real time tracing
 *				  http://processors.wiki.ti.com/index.php/Printf_support_in_compiler
 *				  http://processors.wiki.ti.com/index.php/Tips_for_using_printf 
 */

void platform_write(const char *fmt, ...);
WRITE_info platform_write_configure (WRITE_info write_type);

/*@}*/  /* defgroup */


/** @defgroup    Utility_functions  Utility Functions */
/*@{*/

/** LED operation
*/
typedef enum {

    /** Turn off LED
    */
    PLATFORM_LED_OFF = 0,

    /** Turn on LED
    */
    PLATFORM_LED_ON = 1

} PLATFORM_LED_OP;

/**
 *  @brief       Perform LED operation
 *
 *  @param[in]   led_id    LED ID
 *
 *  @param[in]   operation LED operation
 *
 *  @param[in]   led_class LED Class
 *
 *  @retval     Platform_EOK on Success
 *
 */

Platform_STATUS platform_led(uint32_t led_id, PLATFORM_LED_OP operation, LED_CLASS_E led_class);

/**
 *  @brief       Delay function. The call to this function returns after
 *               specified amount of time.
 *
 *  @param[in]   usecs Delay value in micro-seconds
 *
 *  @retval      Platform_EOK on Success
 *
 */

Platform_STATUS platform_delay(uint32_t usecs);

/**
 *  @brief       Delay function. The call to this function returns after
 *               specified amount of time. It uses the TSCL where 1 TCSL
 *               tick is 1 cycle.
 *
 *  @param[in]   cycles Delay value in clock cycles
 *
 *  @retval      None
 *
 */

void platform_delaycycles(uint32_t cycles);

/*@}*/  /* defgroup */

/** @defgroup    OSAL_functions  OSAL Functions 
* These routines are called from Platform Library and must be implemented by the Application.
*/
/*@{*/

/**
 * ============================================================================
 *  @n@b Osal_platformMalloc
 *
 *  @b  brief
 *  @n  This API is used by platform_library to allocate memory. Applications
 *  	must provide this function and attach it to their memory allocator.
 *
 *  @param[in]  num_bytes
 *      Number of bytes to be allocated.
 *  @param[in]  alignment
 *      byte alignment needed
 *
 *  @return
 *      Allocated block address
 * =============================================================================
 */
uint8_t *Osal_platformMalloc (uint32_t num_bytes, uint32_t alignment);


/**
 * ============================================================================
 *  @n@b Osal_platformFree
 *
 *  @b  brief
 *  @n  Frees up memory allocated using
 *      @a Osal_platformMalloc ()
 *
 *		Applications must provide this function and attach it to ther memory
 *		free handler.
 *
 *  @param[in]  dataPtr
 *      Pointer to the memory block to be cleaned up.
 *
 *  @param[in]  num_bytes
 *      Size of the memory block to be cleaned up.
 *
 *  @return
 *      Not Applicable
 * =============================================================================
 */
void Osal_platformFree (uint8_t *dataPtr, uint32_t num_bytes);

/**
 * ============================================================================
 *  @n@b Osal_platformSpiCsEnter
 *
 *  @b  brief
 *  @n  This API ensures multi-core and multi-threaded
 *      synchronization for the SPI bus.
 *
 *      This is a BLOCKING API.
 *
 *
 *  @return
 *  @n  Handle used to lock critical section
 * =============================================================================
 */
void Osal_platformSpiCsEnter(void);


/**
 * ============================================================================
 *  @n@b Osal_platformSpiCsExit
 *
 *  @b  brief
 *  @n  This API needs to be called to exit a previously
 *      acquired critical section lock using @a Osal_platformSpiCsEnter ()
 *      API. It resets the multi-core and multi-threaded lock,
 *      enabling another process/core to grab the SPI bus.
 *
 *
 *  @return     None
 * =============================================================================
 */
void Osal_platformSpiCsExit (void);



/*@}*/  /* defgroup */

/** @defgroup    Error_handling  Error Handling (Errno values) */
/*@{*/

/**
 *  @brief platform_errno variable may be set to a non-zero value when
 *         a platform library call returns an error.
 *
 *      The errno value is not preserved. The calling application must
 *       save off the value if the platform function call fails.
 */
extern uint32_t platform_errno;

/* Platform errno values */

#define PLATFORM_ERRNO_RESET             0               /* No error */

#define PLATFORM_ERRNO_GENERIC           0x00000001
#define PLATFORM_ERRNO_INVALID_ARGUMENT  0x00000002		/* NULL pointer, argument out of range, etc									*/

#define PLATFORM_ERRNO_PLL_SETUP         0x00000003		/* Error initializing	*/
#define PLATFORM_ERRNO_EEPROM            0x00000004		/* Error initializing	*/
#define PLATFORM_ERRNO_UART              0x00000005		/* Error initializing	*/
#define PLATFORM_ERRNO_LED               0x00000006		/* Error initializing */
#define PLATFORM_ERRNO_I2C               0x00000007		/* Error initializing */
#define PLATFORM_ERRNO_MEMTEST           0x00000008		/* Error initializing */
#define PLATFORM_ERRNO_PHY               0x00000009		/* Error initializing */
#define PLATFORM_ERRNO_NAND              0x0000000a		/* Error initializing */
#define PLATFORM_ERRNO_NOR               0x0000000b		/* Generic error in NOR */
#define PLATFORM_ERRNO_UNSUPPORTED       0x0000000c		/* Functionality not supported */

#define PLATFORM_ERRNO_ECC_FAIL          0x00000010		/* The ECC calculation for the page read doesn't match. The application can try re-reading */
#define PLATFORM_ERRNO_BADFLASHDEV       0x00000011		/* The flash routines did no not recognize the flash manufacturer */
#define PLATFORM_ERRNO_FLASHADDR         0x00000012		/* The block or page number specified does not exist for the Flash */
#define PLATFORM_ERRNO_NANDBBT           0x00000013		/* Could not update the NAND Bad Block Table */
#define PLATFORM_ERRNO_NORADDR           0x00000014		/* Address for NOR does not exist */
#define PLATFORM_ERRNO_NOFREEBLOCKS      0x00000015		/* There were not enough consecutive free blocks to write the data (based on your starting block number) */

#define PLATFORM_ERRNO_DEV_TIMEOUT       0x00000020		/* There was an idle timeout waiting on a device action */
#define PLATFORM_ERRNO_DEV_NAK           0x00000021		/* The device NAK'd the command */
#define PLATFORM_ERRNO_DEV_BUSY          0x00000022		/* The device reported a busy state and could not complete the operation */
#define PLATFORM_ERRNO_DEV_FAIL          0x00000023		/* Device returned a failed status */
#define PLATFORM_ERRNO_PSCMOD_ENABLE     0x00000024		/* Unable to enable the PSC Module */

#define PLATFORM_ERRNO_OOM               0x00000030		/* Out of memory.. tried to allocate RAM but could not */

#define PLATFORM_ERRNO_READTO            0x00000040		/* UART read timeout */

/*@}*/  /* defgroup */

/* DDR3 controller registers */

#define DDR3_BASE_ADDR                   0x21000000
#define DDR_SDCFG                        (DDR3_BASE_ADDR + 0x00000008)
#define DDR_SDRFC                        (DDR3_BASE_ADDR + 0x00000010)
#define DDR_SDTIM1                       (DDR3_BASE_ADDR + 0x00000018)
#define DDR_SDTIM2                       (DDR3_BASE_ADDR + 0x00000020)
#define DDR_SDTIM3                       (DDR3_BASE_ADDR + 0x00000028)
#define DDR_PMCTL                        (DDR3_BASE_ADDR + 0x00000038)
#define RDWR_LVL_RMP_WIN                 (DDR3_BASE_ADDR + 0x000000D4)
#define RDWR_LVL_RMP_CTRL                (DDR3_BASE_ADDR + 0x000000D8)
#define RDWR_LVL_CTRL                    (DDR3_BASE_ADDR + 0x000000DC)
#define DDR_ZQCFG                        (DDR3_BASE_ADDR + 0x000000C8)
#define DDR_PHYCTRL                      (DDR3_BASE_ADDR + 0x000000E4)

#define DDR3_PLLCTL0                     0x02620330
#define DDR3_PLLCTL1                     0x02620334

#define DDR3_CONFIG_REG_0                0x02620404
#define DDR3_CONFIG_REG_1                0x02620408
#define DDR3_CONFIG_REG_12               0x02620434
#define DDR3_CONFIG_REG_23               0x02620460
#define DDR3_CONFIG_REG_24               0x02620464

#endif  /* PLATFORM_UTILS_H_ */
