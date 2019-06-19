/*
 * Copyright (C) 2018 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_stm32_common
 * @ingroup     drivers_periph_eeprom
 * @{
 *
 * @file
 * @brief       Low-level eeprom driver implementation
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 * @author      Oleg Artamonov <oleg@unwds.com>
 * @author      Francisco Molina <francois-xavier.molina@inria.fr>
 *
 * @}
 */

#include <assert.h>

#include "cpu.h"
#include "periph/eeprom.h"

#if defined(MODULE_MTD)
#include "board.h"
#include "mtd.h"
#include <string.h>
#endif

#define ENABLE_DEBUG        (0)
#include "debug.h"

extern void _lock(void);
extern void _unlock(void);
extern void _wait_for_pending_operations(void);

#ifndef EEPROM_START_ADDR
#error "periph/eeprom: EEPROM_START_ADDR is not defined"
#endif

#if defined(CPU_MODEL_STM32L151CB)
#define ALIGN_MASK          (0x00000003)
#define BYTE_MASK           (0xFF)
#define BYTE_BITS           (0x08)

static void _erase_word(uint32_t addr)
{
    /* Wait for last operation to be completed */
    _wait_for_pending_operations();

    /* Write "00000000h" to valid address in the data memory" */
    *(__IO uint32_t *)addr = 0x00000000;
}

static void _write_word(uint32_t addr, uint32_t data)
{
    /* Wait for last operation to be completed */
    _wait_for_pending_operations();

    *(__IO uint32_t *)addr = data;
}
#endif

static void _write_byte(uint32_t addr, uint8_t data)
{
    /* Wait for last operation to be completed */
    _wait_for_pending_operations();

#if defined(CPU_MODEL_STM32L151CB)
    /* stm32l1xxx cat 1 can't write NULL bytes RefManual p79*/
    uint32_t tmp = 0;
    uint32_t data_mask = 0;

    if (data != (uint8_t)0x00) {
        *(__IO uint8_t *)addr = data;
    }
    else {
        tmp = *(__IO uint32_t *)(addr & (~ALIGN_MASK));
        data_mask = BYTE_MASK << ((uint32_t)(BYTE_BITS * (addr & ALIGN_MASK)));
        tmp &= ~data_mask;
        _erase_word(addr & (~ALIGN_MASK));
        _write_word((addr & (~ALIGN_MASK)), tmp);
    }
#else
    *(__IO uint8_t *)addr = data;
#endif
}

size_t eeprom_read(uint32_t pos, void *data, size_t len)
{
    assert(pos + len <= EEPROM_SIZE);

    uint8_t *p = data;

    DEBUG("Reading data from EEPROM at pos %" PRIu32 ": ", pos);
    for (size_t i = 0; i < len; i++) {
        _wait_for_pending_operations();
        *p++ = *(__IO uint8_t *)(EEPROM_START_ADDR + pos++);
        DEBUG("0x%02X ", *p);
    }
    DEBUG("\n");

    return len;
}

size_t eeprom_write(uint32_t pos, const void *data, size_t len)
{
    assert(pos + len <= EEPROM_SIZE);

    uint8_t *p = (uint8_t *)data;

    _unlock();

    for (size_t i = 0; i < len; i++) {
        _write_byte((EEPROM_START_ADDR + pos++), *p++);
    }

    _lock();

    return len;
}

#if defined(MODULE_MTD)
/*** DUMMY MTD ***/
#ifndef SECTOR_COUNT
#define SECTOR_COUNT 2
#endif
#ifndef PAGE_PER_SECTOR
#define PAGE_PER_SECTOR 8
#endif
#ifndef PAGE_SIZE
#define PAGE_SIZE 128
#endif


static int _init(mtd_dev_t *dev)
{
    (void)dev;
    // No memory erase. init is invoked before mount in little_fs.
    return 0;
}

static int _read(mtd_dev_t *dev, void *buff, uint32_t addr, uint32_t size)
{
    (void)dev;

    return eeprom_read(addr, buff, size);
}

static int _write(mtd_dev_t *dev, const void *buff, uint32_t addr, uint32_t size)
{
    (void)dev;

    return eeprom_write(addr, buff, size);
}

static int _erase(mtd_dev_t *dev, uint32_t addr, uint32_t size)
{
    (void)dev;
    (void)addr;
    (void)size;
    return 0;
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

mtd_dev_t mtd0_dev = {
    .driver = &mtd_driver,
    .sector_count = SECTOR_COUNT,
    .pages_per_sector = PAGE_PER_SECTOR,
    .page_size = PAGE_SIZE
};

mtd_dev_t * const mtd0 = &mtd0_dev;

#endif
