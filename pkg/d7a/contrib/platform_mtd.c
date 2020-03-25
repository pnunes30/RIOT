/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     pkg_d7a
 * @file
 * @brief       dummy MTD driver implementation (imported from tests-littlefs)
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */

#if defined(MODULE_MTD)

#if !defined(MTD0) || !defined(MTD1)

#include "periph_cpu.h"
#include "board.h"
#include "mtd.h"
#include <errno.h>
#include <string.h>

#include "framework/inc/fs.h"
#include "framework/inc/errors.h"


#define ENABLE_DEBUG (0)

#if (ENABLE_DEBUG)
#include "debug.h"

static void log_print_data(uint8_t* message, uint32_t length);
#define DPRINT(...) DEBUG_PRINT(__VA_ARGS__)
#define DPRINT_DATA(...) log_print_data(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINT_DATA(...)
#endif

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
} mtd_dev_dummy_t;

#if (ENABLE_DEBUG)
static void log_print_data(uint8_t* message, uint32_t length)
{
    for( uint32_t i=0 ; i<length ; i++ )
    {
        printf(" %02X", message[i]);
    }
}
#endif

static int _init(mtd_dev_t *dev)
{
    (void)dev;

    // No memory erase. init is invoked before mount in little_fs.
    return SUCCESS;
}

static int _read(mtd_dev_t *dev, void *buff, uint32_t addr, uint32_t size)
{
    if (addr + size > ((mtd_dev_dummy_t *)dev)->size) {
        return -EOVERFLOW;
    }
    memcpy(buff, ((mtd_dev_dummy_t *)dev)->buffer + addr, size);

    return SUCCESS;
}

static int _write(mtd_dev_t *dev, const void *buff, uint32_t addr, uint32_t size)
{
    if (addr + size > ((mtd_dev_dummy_t *)dev)->size) {
        return -EOVERFLOW;
    }
    if (size > PAGE_SIZE) {
        return -EOVERFLOW;
    }
    memcpy(((mtd_dev_dummy_t *)dev)->buffer + addr, buff, size);

    return SUCCESS;
}

static int _erase(mtd_dev_t *dev, uint32_t addr, uint32_t size)
{
    if (size % (PAGE_PER_SECTOR * PAGE_SIZE) != 0) {
        return -EOVERFLOW;
    }
    if (addr % (PAGE_PER_SECTOR * PAGE_SIZE) != 0) {
        return -EOVERFLOW;
    }
    if (addr + size > ((mtd_dev_dummy_t *)dev)->size) {
        return -EOVERFLOW;
    }
    memset(((mtd_dev_dummy_t *)dev)->buffer + addr, 0xff, size);

    return SUCCESS;
}

static int _power(mtd_dev_t *dev, enum mtd_power_state power)
{
    (void)dev;
    (void)power;
    return 0;
}

static const mtd_desc_t mtd_driver = {
    .init = _init,
    .read = _read,
    .write = _write,
    .erase = _erase,
    .power = _power,
};

#define METADATA_SIZE (4 + 4 + (12 * FRAMEWORK_FS_FILE_COUNT))

/*** simple RAM-based blockdevice ***/
extern uint8_t d7ap_fs_metadata[METADATA_SIZE];
extern uint8_t d7ap_files_data[FRAMEWORK_FS_PERMANENT_STORAGE_SIZE];
extern uint8_t d7ap_volatile_files_data[FRAMEWORK_FS_VOLATILE_STORAGE_SIZE];

#if !defined(MTD0)
static uint8_t dummy_mtd0[PAGE_PER_SECTOR * PAGE_SIZE * SECTOR_COUNT] __attribute__((section(".noinit")));

mtd_dev_dummy_t dummy_mtd0_dev = {
    .dev.driver = &mtd_driver,
    .dev.sector_count = SECTOR_COUNT,
    .dev.pages_per_sector = PAGE_PER_SECTOR,
    .dev.page_size = PAGE_SIZE,
    .size = sizeof(d7ap_fs_metadata),
    .buffer = d7ap_fs_metadata
};

mtd_dev_t * const mtd0 = (mtd_dev_t* const) &dummy_mtd0_dev;
#endif

#if !defined(MTD1)
static uint8_t dummy_mtd1[PAGE_PER_SECTOR * PAGE_SIZE * SECTOR_COUNT] __attribute__((section(".noinit")));

mtd_dev_dummy_t dummy_mtd1_dev = {
    .dev.driver = &mtd_driver,
    .dev.sector_count = SECTOR_COUNT,
    .dev.pages_per_sector = PAGE_PER_SECTOR,
    .dev.page_size = PAGE_SIZE,
    .size = sizeof(d7ap_files_data),
    .buffer = d7ap_files_data
};

mtd_dev_t * const mtd1 = (mtd_dev_t* const) &dummy_mtd1_dev;
#endif

#if !defined(MTD2)
static uint8_t dummy_mtd2[PAGE_PER_SECTOR * PAGE_SIZE * SECTOR_COUNT] __attribute__((section(".noinit")));

mtd_dev_dummy_t dummy_mtd2_dev = {
    .dev.driver = &mtd_driver,
    .dev.sector_count = SECTOR_COUNT,
    .dev.pages_per_sector = PAGE_PER_SECTOR,
    .dev.page_size = PAGE_SIZE,
    .size = sizeof(d7ap_volatile_files_data),
    .buffer = d7ap_volatile_files_data
};

mtd_dev_t * const mtd2 = (mtd_dev_t* const) &dummy_mtd2_dev;
#endif

#endif /* !defined(MTD0) || !defined(MTD1) */

#endif /*MODULE_MTD*/
