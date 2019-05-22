/*
 * Copyright (C) 2018 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @ingroup     drivers_periph_mtd
 * @{
 *
 * @file
 * @brief       dummy MTD driver implementation (imported from tests-littlefs)
 *
 * @author      Cortus <sw@cortus.com>
 *
 * @}
 */


#include "periph_cpu.h"

#if defined(MODULE_MTD)

#include "board.h"
#include "mtd.h"
#include <errno.h>
#include <string.h>

#if defined(MODULE_MTD_SPI_NOR)
# if !defined(USE_PERIPH_CORTUS_MTD_FPGA_M25P32)
# error "MODULE_MTD_SPI_NOR requires explicit enable of FPGA M25P32"
# endif

#include "mtd_spi_nor.h"

static mtd_spi_nor_t cortus_mtd0_dev = {
    .base = {
        .driver = &mtd_spi_nor_driver,
        .page_size        = APS_FPGA_M25P32_PAGE_SIZE,
        .pages_per_sector = APS_FPGA_M25P32_PAGES_PER_SECTOR,
        .sector_count     = APS_FPGA_M25P32_SECTOR_COUNT,
      },
    .opcode = &mtd_spi_nor_opcode_default,
    .spi  = APS_FPGA_M25P32_DEV,
    .cs   = APS_FPGA_M25P32_CS,
    .addr_width = 3,
    .mode = APS_FPGA_M25P32_MODE,
    .clk  = APS_FPGA_M25P32_CLK,
  };

#else
/*** DUMMY MTD ***/

/* Test mock object implementing a simple RAM-based mtd */
#ifndef SECTOR_COUNT
#define SECTOR_COUNT 32
#endif
#ifndef PAGE_PER_SECTOR
#define PAGE_PER_SECTOR 4
#endif
#ifndef PAGE_SIZE
#define PAGE_SIZE 64
#endif

typedef struct {
    mtd_dev_t dev;
    uint32_t size;
    uint8_t* buffer;
} mtd_dev_cortus_t;


static uint8_t aps_dummy_mtd0[PAGE_PER_SECTOR * PAGE_SIZE * SECTOR_COUNT] __attribute__((section(".noinit")));
static uint8_t aps_dummy_mtd1[PAGE_PER_SECTOR * PAGE_SIZE * SECTOR_COUNT] __attribute__((section(".noinit")));


static int _init(mtd_dev_t *dev)
{
    (void)dev;

    // No memory erase. init is invoked before mount in little_fs.
    return 0;
}

static int _read(mtd_dev_t *dev, void *buff, uint32_t addr, uint32_t size)
{
    if (addr + size > ((mtd_dev_cortus_t *)dev)->size) {
        return -EOVERFLOW;
    }
    memcpy(buff, ((mtd_dev_cortus_t *)dev)->buffer + addr, size);

    return size;
}

static int _write(mtd_dev_t *dev, const void *buff, uint32_t addr, uint32_t size)
{
    if (addr + size > ((mtd_dev_cortus_t *)dev)->size) {
        return -EOVERFLOW;
    }
    if (size > PAGE_SIZE) {
        return -EOVERFLOW;
    }
    memcpy(((mtd_dev_cortus_t *)dev)->buffer + addr, buff, size);

    return size;
}

static int _erase(mtd_dev_t *dev, uint32_t addr, uint32_t size)
{
    if (size % (PAGE_PER_SECTOR * PAGE_SIZE) != 0) {
        return -EOVERFLOW;
    }
    if (addr % (PAGE_PER_SECTOR * PAGE_SIZE) != 0) {
        return -EOVERFLOW;
    }
    if (addr + size > ((mtd_dev_cortus_t *)dev)->size) {
        return -EOVERFLOW;
    }
    memset(((mtd_dev_cortus_t *)dev)->buffer + addr, 0xff, size);

    return 0;
}

static int _power(mtd_dev_t *dev, enum mtd_power_state power)
{
    (void)dev;
    (void)power;
    return 0;
}

static const mtd_desc_t cortus_mtd_driver = {
    .init = _init,
    .read = _read,
    .write = _write,
    .erase = _erase,
    .power = _power,
};

mtd_dev_cortus_t cortus_mtd0_dev = {
    .dev.driver = &cortus_mtd_driver,
    .dev.sector_count = SECTOR_COUNT,
    .dev.pages_per_sector = PAGE_PER_SECTOR,
    .dev.page_size = PAGE_SIZE,
    .size = sizeof(aps_dummy_mtd0),
    .buffer = aps_dummy_mtd0
};

mtd_dev_cortus_t cortus_mtd1_dev = {
    .dev.driver = &cortus_mtd_driver,
    .dev.sector_count = SECTOR_COUNT,
    .dev.pages_per_sector = PAGE_PER_SECTOR,
    .dev.page_size = PAGE_SIZE,
    .size = sizeof(aps_dummy_mtd1),
    .buffer = aps_dummy_mtd1

};

#endif /*DUMMY|FPGA*/

mtd_dev_t * const mtd0 = (mtd_dev_t* const) &cortus_mtd0_dev;
mtd_dev_t * const mtd1 = (mtd_dev_t* const) &cortus_mtd1_dev;

#endif /*MODULE_MTD*/
