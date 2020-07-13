/*
 * Copyright (C) 2020 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup    cpu_aps
 * @{
 *
 * @brief      Low-Level flash driver header
 *
 * @author     matthieu.saignemorte@cortus.com
 */

#ifndef CORTUS_FLASH_H_
#define CORTUS_FLASH_H_

#include "periph_cpu.h"

/**
 * @brief      Erase flash (granularity for an erase is by sector)
 *
 * If the start address is not sector-aligned, the start of the first sector
 * and the end of the last sector will be erased !
 *
 * @param[in] addr          Starting address
 * @param[in] size          Number of words to erase
 * @return                  ERROR/SUCCESS
 */
int FLASH_ERASE_SECTOR(uint32_t addr, uint32_t size);


/**
 * @brief      Program a chunk of the FLASH memory (in bytes).
 *
 * @note       The source and destination buffers have no specific alignment constraints.
 *
 * @param[in] addr          Starting address
 * @param[in] dst           Pointer to the input data buffer
 * @param[in] size          Number of bytes to update
 * @return                  ERROR/SUCCESS
 */
int FLASH_PROGRAM(uint32_t addr, const void *data, uint32_t size);


/**
 * @brief      Read flash
 *
 * @param[in] addr          Starting address
 * @param[in] dst           Pointer to the output data buffer
 * @param[in] size          Number of words to read
 * @return                  ERROR/SUCCESS
 */
int FLASH_READ(uint32_t addr, uint32_t *dst, uint32_t size);


/**
 * @brief      Erase the selected bank
 *
 * @param[in] bank          0 for bank_0 and 1 for bank_1
 * @return                  ERROR/SUCCESS
 */
int FLASH_ERASE_BANK(uint8_t bank);


/**
 * @brief      Modify the boot key to swap bank next time we boot
 *
 * @param[in] bank          Bank number (0 or 1)
 * @return                  ERROR/SUCCESS
 */
int FLASH_BANK_SWAP(int bank);


/**
 * @brief      Get the bank from which we're executing the actual code
 *
 * @return                  Bank number(0 or 1)/error
 */
int FLASH_GET_CURRENT_BANK(void);


#endif /* CORTUS_FLASH_H_ */

/** @}*/
