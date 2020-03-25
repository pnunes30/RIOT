/*
 * Copyright (C) 2017 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup cortus_fpga
 * @{
 */

/**
 * @file
 * @brief       cortus_fpga board initialization
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 */

#include "board.h"
#include "periph/gpio.h"
#include "stdio_uart.h"

/*
 * @brief       default aps mtd0 filesystem
 */
#if defined(MODULE_VFS) && defined (MTD_VFS)

#if defined(MODULE_LITTLEFS)
#include "fs/littlefs_fs.h"

#warning "FIXME littlefs: 1 sector per file (board.h)????"

static littlefs_desc_t default_fs_desc = {
    .lock = MUTEX_INIT,
    .dev  = 0,              //force init
#ifdef MODULE_MTD_SPI_NOR
    .base_addr = APS_FPGA_M25P32_BASE_SECTOR,
#endif
};
#define DEFAULT_FS_DRIVER littlefs_file_system

#elif defined(MODULE_SPIFFS)
#include "fs/spiffs_fs.h"
static spiffs_desc_t default_fs_desc = {
    .lock = MUTEX_INIT,
    .dev  = 0,              //force init
#ifdef MODULE_MTD_SPI_NOR
#if (SPIFFS_SINGLETON == 0)
    .base_addr   = APS_FPGA_M25P32_BASE_ADDR,
    .block_count = (APS_FPGA_M25P32_SECTOR_COUNT-APS_FPGA_M25P32_BASE_SECTOR),  /**< Number of blocks in current partition*/
                                                                                /*  if 0, the mtd number of sector is used */
#else
#error "SPIFFS_SINGLETON!=0 does not allow to set base_addr. It's probably not a good idea to erase the fpga bitmap below 0x280000..."
#endif
#endif
};
#define DEFAULT_FS_DRIVER spiffs_file_system

#else
#error "No vfs filesystem defined. Please enable SPIFFS or LITTLEFS"
#endif

#ifdef DEFAULT_FS_DRIVER
vfs_mounttab_t aps_mtd0_mount = {
    .fs             = &DEFAULT_FS_DRIVER,
    .mount_point    = "/mtd0",
    .private_data   = &default_fs_desc,
};
#define aps_fs_init() { default_fs_desc.dev = MTD_VFS; }
#endif

#endif /*defined(MODULE_VFS) && defined(MTD_0)*/

#ifndef aps_fs_init
#define aps_fs_init()  /*noop*/
#endif

/*
 * @brief       default aps tmpfs filesystem
 */
#if defined(MODULE_VFS) && defined (MODULE_TMPFS)
#include "fs/tmpfs.h"
static tmpfs_t tmpfs_desc = {
    .lock = MUTEX_INIT,
};
vfs_mounttab_t aps_tmpfs_mount = {
    .fs = &tmpfs_file_system,
    .mount_point = "/tmp",
    .private_data = &tmpfs_desc,
};

#endif /*MODULE_TMPFS*/


void board_init(void)
{
    /* first initialize STDIO over UART (for early printf...)
     * !Be warned about xtimer initialization conflict in tx_isr mode!
     */
    stdio_init();

    /* initialize the CPU */
    cpu_init();

    //__watchdog_init(); // done in periph_init ??
    aps_fs_init();
}
