/*
 * Copyright (C) 2020 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @{
 *
 * @brief       Low-Level flash driver implementation
 *
 * @author      matthieu.saignemorte@cortus.com
 */

/* FLASH MEMORY description:
 *
 * - 2 banks of 255 1/2 KBytes each (511 sectors)
 * - 1024 Bytes (2 sectors) reserved for calibration and configuration of HW
 *                  ( bootkey, radio filter coefficient, Crystal offset ...)
 * - Total number of sectors in main array (full flash) : 1024
 * - 512 Bytes (128 words) per sector
 *
 * Software addresses for CFG and CLI sector are not set to
 * 0xE0000000 and 0xE0000200 but are right after the bank1 (follow hardware addr).
 *
 *             +------------------------+   0x80080000
 *             |________________________|   Calibration   sector (512 Bytes)
 *             |            |  BOOTKEY  |   Configuration sector (512 Bytes)
 *             +------------------------+   0x8007FC00
 *             |                        |
 *             |                        |
 *             |                        |
 *             |                        |
 *             |                        |
 *             |                        |
 *             |                        |         BANK 1 (255 1/2 KBytes)
 *             |                        |
 *             |                        |
 *             |       OTA - UPDATE     |
 *             |                        |
 *             |                        |
 *             +------------------------+
 *             |         VECTORS        |
 *             +------------------------+   0x8003FE00
 *             |                        |
 *             |                        |
 *             |                        |
 *             |                        |
 *             |                        |
 *             |                        |
 *             |                        |         BANK 0 (255 1/2 KBytes)
 *             |                        |
 *             |                        |
 *             |       APPLICATION      |
 *             |                        |
 *             |                        |
 *             +------------------------+
 *             |         VECTORS        |
 *             +------------------------+   0x80000000
 *
 *
 * NOTES:
 * The two banks are contiguous, so that, if OTA updates are not used, the full
 * 512 KBytes can be used.
 *
 * Calibration and Configuration blocks:
 * Modulo the actual Flash sector size - this will be two Flash sectors (one
 * “stolen” from each bank). The calibration is never erased and contains the
 * “cold” start data for the radio. The configuration is changed according to
 * the protocol and contains the gaussian filter parameters, for example.
 * The JTAG fuse bit and JTAG ID are contained in the calibration block, as
 * well as the boot select bits.
 *
 * For now, using flash APIs allow the user to erase/program/read CFG and CLI
 * sectors.
 *
 * BootKey:
 * The bootkey is 256 bits long. It is used to know what bank is going to be
 * used during running time. Even number of bits set (clear) then select bank 0,
 * odd number of bits set (clear) then select bank 1 @ 0x8000 0000.
 *
 *
 * Improvements suggestions:
 *
 * PARAM 'bank' is an int type and used as boolean. Might be better
 * to use a struct to have more info about bank.
 *
 */

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "cortus_flash.h"
#include "machine/flash_ctl.h"
#include "machine/cpu.h"

#include "xtimer.h"

#include "debug.h"

#if ENABLE_DEBUG
static void log_print_data(uint8_t* message, uint32_t length);
    #define DEBUG_DATA(...) log_print_data(__VA_ARGS__)
#else
    #define DEBUG_DATA(...)
#endif

#if (ENABLE_DEBUG)
static void log_print_data(uint8_t* message, uint32_t length)
{
    for( uint32_t i=0 ; i<length ; i++ )
    {
        printf(" %02X", message[i]);
    }
}
#endif

#define FLASH_BASE_ADDR             0x80000000
#define FLASH_TOP_ADDR              0x80080000
#define FLASH_CFG_CAL_START_ADDR    0x8007FC00

#define FLASH_BOOT_KEY_ADDR         0x8007FC00
#define FLASH_BOOT_KEY_NWORDS       8

#define FLASH_BANK0_ADDR            0x80000000
#define FLASH_BANK1_ADDR            0x8003FE00
#define FLASH_BANK_SIZE             0x0003FE00

#define FLASH_MAIN_SECTORS          (1 << 10)       // 1024 sectors in main
#define FLASH_SECTOR_SIZE_BYTES     (1 << 9)        // 512Bytes
#define FLASH_SECTOR_HIGHT          (1 << 7)        // 128 (128x32bits)
#define FLASH_ROW_SIZE              (1 << 6)        // 64

// FIXME : No explanation for the define's values ...
#define FLASH_MAIN                  0x0  // Main sector of the flash (bank 0 and 1)
#define FLASH_ACCESS_TIME           0x7  // Default access time needed ?

#define __RAMFUNC                   __attribute__((section(".ram")))
#define CPU_FREQ                    32 //MHz

/* TODO: Decide if it's mandatory/necessary or not and link it with Error code
 * If defined, it'll add delay. How much? Is it good ?
 */
//#define VERIFICATIONS               0 // Enable erase and program flash verifications
#define SUCCESS                     0
#define ERROR                      -1
#define ERROR_FLASH_SIZE           -2
#define ERROR_INVALID_ADDR         -3

#define flash                       ((volatile unsigned int*)0x80000000)


/* Waiting for specific and required flash timing. */
static void flash_wait_complete(uint32_t us_time)
{
    xtimer_usleep(us_time);
}


/**
 * @brief      Erase sectors (sector size : 128x32bits)
 *
 * @param[in] start_sector_index    First sector to erase
 * @param[in] sector_number         Number of sectors to erase
 * @return                          ERROR/SUCCESS
 */
__RAMFUNC static int aps_flash_erase_sectors(uint32_t start_sector_index, uint32_t sector_number)
{
  uint32_t remaining_sector = 0;
  uint32_t sector_add = 0;

  /* Hardware sector address (sector position * 128) */
  sector_add = start_sector_index * FLASH_SECTOR_HIGHT;
  remaining_sector = sector_number;


  //INT_Disable();
  cpu_int_disable();

  // Config to erase inside the MAIN SECTOR:
  flash_ctl->ctrreg = FLASH_MAIN | FLASH_ERASE | FLASH_WR_EN;
  flash_ctl->tACC = FLASH_ACCESS_TIME;

  // From hardware POV, 1 sector is 128 words (128*32 bits)
  while(remaining_sector)
  {
    flash[sector_add] = 0x0; // Will initiate sector erase
    sector_add = sector_add + FLASH_SECTOR_HIGHT;
    remaining_sector = remaining_sector - 1;
  }

  flash_ctl->ctrreg = FLASH_MAIN;

#ifdef VERIFICATIONS
  /* VERIFICATION */
  // Config to read inside the MAIN SECTOR done above (FLASH_MAIN)

  // Verify every words of every modified sector.
  sector_add = start_sector_index * FLASH_SECTOR_HIGHT;
  for (uint32_t i = sector_add; i < (sector_add + (FLASH_SECTOR_HIGHT * sector_number)); i++)
  {
    if (flash [i] != 0xFFFFFFFF)
    {
      DEBUG("i = %08lX value = %08lX\n", i, flash [i]);
      flash_ctl->ctrreg = FLASH_MAIN;
      //flash_ctl->tACC = 0x0;
      return ERROR;
    }
  }

  //FIXME: Why 1us ?
  // Wait 1us after a negedge READ
  flash_wait_complete(1);

  flash_ctl->ctrreg = FLASH_MAIN;
  //FIXME : Why set tACC to 0 ? Doesn't work when executing in flash
  //flash_ctl->tACC = 0x0;
#endif

  //INT_Enable();
  cpu_int_enable();

  return SUCCESS;
}


/**
 * @brief      Program the flash
 *
 * @param[in] addr              Starting address
 * @param[in] data              Pointer to the input data buffer
 * @param[in] size              Number of words to write
 * @return                      ERROR/SUCCESS
 *
 * Notes: Programming the flash can only be done after erasing the actual sector
 * or only when setting bits from 1 to 0 (used for boot key).
 * The address must be aligned and the input data must be the
 * length of a word.
 */
__RAMFUNC static int aps_flash_program(uint32_t addr, uint32_t *data, uint32_t size)
{
  uint32_t remaining_words = size;

  /* Hardware flash addresses are going from 0x0 to 0x20000 but
  Software flash addresses are going from 0x0 to 80000 so we right shift by 2 */
  uint32_t address_index = addr >> 2;
  uint32_t data_index = 0;

  //INT_Disable();
  cpu_int_disable();

  // Config to write inside the MAIN SECTOR:
  flash_ctl->ctrreg = FLASH_MAIN | FLASH_WR_EN;
  flash_ctl->tACC = FLASH_ACCESS_TIME;

  while (remaining_words > 0)
  {
    if (remaining_words >= FLASH_ROW_SIZE)
    {
      for (uint8_t i = 0; i < FLASH_ROW_SIZE; i++)
      {
        flash[address_index] = *(data + data_index);
        address_index++;
        data_index++;
      }
      remaining_words = remaining_words - FLASH_ROW_SIZE;
    }
    else {
      for (uint8_t i = 0; i < remaining_words; i++)
      {
        flash[address_index] = *(data + data_index);
        address_index++;
        data_index++;
      }
      remaining_words = 0;
    }

    // FIXME : Why 15us ?
    // Delay of 15us between a write ROW operation
    flash_wait_complete(15);
  }

  flash_ctl->ctrreg = FLASH_MAIN;

#ifdef VERIFICATIONS
  /* VERIFICATION */
  // Config to read MAIN SECTOR done above (FLASH_MAIN)

  // Verify
  for (uint32_t j = 0; j < size; j++) {
      if (flash [(addr >> 2) + j] != *(data + j)) {
          return ERROR;
      }
  }

  flash_ctl->ctrreg = 0x0;
  //FIXME : Why set tACC to 0 ? Doesn't work when executing in flash
  //flash_ctl->tACC = 0x0;
#endif

  //INT_Enable();
  cpu_int_enable();

  return SUCCESS;
}


/**
 * @brief      Read the flash
 *
 * @param[in] addr              Starting address
 * @param[in] buffer            Pointer to the output data buffer
 * @param[in] size              Number of words to read
 * @return                      ERROR/SUCCESS
 */
__RAMFUNC static int aps_flash_read(uint32_t addr, uint32_t *buffer, uint16_t size)
{
  /* Hardware flash addresses are going from 0x0 to 0x1FF00 but
  Software flash addresses are going from 0x0 to 7FC00 so we right shift by 2 */
  uint32_t add_index = addr >> 2;

  //INT_Disable();
  cpu_int_disable();

  // Config to read MAIN SECTOR:
  flash_ctl->ctrreg = FLASH_MAIN;
  flash_ctl->tACC = FLASH_ACCESS_TIME;

  for (uint32_t j = 0; j < size; j++) {
      *(buffer + j) = flash[add_index + j];
  }

  flash_ctl->ctrreg = FLASH_MAIN;
  //FIXME : Why set tACC to 0 ? Doesn't work when executing in flash
  //flash_ctl->tACC = 0x0;

  //INT_Enable();
  cpu_int_enable();

  return SUCCESS;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/


int FLASH_ERASE_SECTOR(uint32_t addr, uint32_t size)
{
  int ret = ERROR;
  int sector_position = 0;
  int sector_counter = 0;

  /* Address boundary check */
  if ((addr >= FLASH_TOP_ADDR) || (addr < FLASH_BASE_ADDR)) {
    DEBUG("[flash] Error: address is not mapped in the internal flash memory\n");
    return ERROR_INVALID_ADDR;
  }

  sector_position = (addr - FLASH_BASE_ADDR) / FLASH_SECTOR_SIZE_BYTES;

  /* number of sectors to erase */
  sector_counter = size / FLASH_SECTOR_HIGHT;

  if ((sector_position + sector_counter) > FLASH_MAIN_SECTORS) {
    DEBUG("[flash] Error: sector to erase exceeds flash memory\n");
    return ERROR_FLASH_SIZE;
  }

  ret = aps_flash_erase_sectors(sector_position, (sector_counter + 1));

  if (ret == 0) {
    ret = SUCCESS;
  }else {
    DEBUG("[flash] Error: unable to erase the flash\n");
    ret = ERROR;
  }

  return ret;
}


int FLASH_PROGRAM(uint32_t addr, const void *data, uint32_t size)
{
  if ((addr >= FLASH_TOP_ADDR) || (addr < FLASH_BASE_ADDR) || (data == NULL))
  {
    DEBUG("[flash] Error: invalid address or data buffer\n");
    return ERROR_INVALID_ADDR;
  }

  if ((addr + size) >= FLASH_TOP_ADDR)
  {
    DEBUG("[flash] Error: size exceeds the flash memory\n");
    return ERROR_FLASH_SIZE;
  }

  int ret = 0;
  int remaining = size;
  int len = 0;
  uint8_t *src_addr = (uint8_t *) data;
  uint32_t * sector_cache = calloc(FLASH_SECTOR_HIGHT, sizeof(uint32_t));

  if (sector_cache != NULL)
  {
    do
    {
      // Sector is round down
      uint32_t sector = (addr - FLASH_BASE_ADDR) / FLASH_SECTOR_SIZE_BYTES;
      //address_sector = sector * FLASH_SECTOR_SIZE_BYTES
      int fl_offset = (addr - FLASH_BASE_ADDR) - (sector * FLASH_SECTOR_SIZE_BYTES);
      len = ((FLASH_SECTOR_SIZE_BYTES - fl_offset) < remaining) ? (FLASH_SECTOR_SIZE_BYTES - fl_offset) : remaining;

      /* Load from the flash into the cache */
      ret = aps_flash_read((sector * FLASH_SECTOR_SIZE_BYTES), sector_cache, FLASH_SECTOR_HIGHT);
      /* Update the cache from the source */
      memcpy((uint8_t *)sector_cache + fl_offset, src_addr, len);
      /* Erase the page, and write the cache */
      ret = aps_flash_erase_sectors(sector, 1);
      if (ret != SUCCESS) {
        DEBUG("[flash] Error erasing at 0x%08lx\n", ((sector * FLASH_SECTOR_SIZE_BYTES) + FLASH_BASE_ADDR));
      }
      else {
        ret = aps_flash_program((sector * FLASH_SECTOR_SIZE_BYTES), sector_cache, FLASH_SECTOR_HIGHT);
        if (ret != SUCCESS) {
          DEBUG("[flash] Error writing %d bytes at 0x%08lx\n", FLASH_SECTOR_SIZE_BYTES, ((sector * FLASH_SECTOR_SIZE_BYTES) + FLASH_BASE_ADDR));
        }
        else {
          addr = addr + len;
          src_addr += len;
          remaining = remaining - len;
        }
      }

    } while ((ret == SUCCESS) && (remaining > 0));
    free(sector_cache);
  }
  return ret;
}


int FLASH_READ(uint32_t addr, uint32_t *dst, uint32_t size)
{
  if ((addr >= FLASH_TOP_ADDR) || (addr < FLASH_BASE_ADDR) || (dst == NULL))
  {
    DEBUG("[flash] Error: invalid address or data buffer\n");
    return ERROR_INVALID_ADDR;
  }
  else if ((addr % 4) != 0) // Word basis flash
  {
    DEBUG("[flash] Error: address must be word aligned\n");
    return ERROR_INVALID_ADDR;
  }
  else
  {
    return (aps_flash_read((addr - FLASH_BASE_ADDR), dst, size));
  }
}

int FLASH_ERASE_BANK(uint8_t bank)
{
    int ret = ERROR;
    if (bank == 0)
    {
        ret = aps_flash_erase_sectors( 0x00, (FLASH_SECTOR_SIZE_BYTES - 1) );
    }
    else if (bank == 1)
    {
        // Do not erase configuration & calibration sectors
        ret = aps_flash_erase_sectors( (FLASH_SECTOR_SIZE_BYTES - 1), (FLASH_SECTOR_SIZE_BYTES - 1));
    }
    else
    {
        DEBUG("[flash] Error undefined bank\n");
        ret = ERROR;
    }

    if (ret == 0)
    {
      ret = SUCCESS;
    }
    else
    {
      DEBUG("[flash] Error: unable to erase the flash\n");
      ret = ERROR;
    }
    return ret;
}


/* Notes about the boot key:
 * Boot key is a 8 words key set to 0xFFFFFFFF by default.
 * If the number of bits set to 1 is even, then we'll boot on bank0. Bank1 otherwise.
 * To avoid brick state, we should never erase the configuration sector in which
 * the boot key is stored. Thus we can't program it because it would mean, erase then program.
 * The flash authorize us to program it but only in one direction: setting bits
 * from 1 to 0 (0 to 0 also) but not from 0 to 1. This is what we do here.
 */
int FLASH_BANK_SWAP(int bank)
{
  int ret = ERROR;
  uint16_t bit_index = 0;
  uint16_t max_bit_index = 0;
  uint8_t bits_set = 0;
  uint8_t word_index = 0;
  uint32_t boot_key[FLASH_BOOT_KEY_NWORDS];
  uint32_t addr = 0;

  // Read boot key
  ret = aps_flash_read((FLASH_BOOT_KEY_ADDR - FLASH_BASE_ADDR), &boot_key[0], FLASH_BOOT_KEY_NWORDS);

  // Check if we went through the entire boot key already (256 bits set to "0")
  if (boot_key[7] == 0x0)
  {
      // fill the new boot key value with 0xFFFFFFFF (like an erase)
      for (int i=0; i<FLASH_BOOT_KEY_NWORDS; i++)
          boot_key[i] = 0xFFFFFFFF;

      // Program the boot key (can't do only an erase since it will erase the entire sector)
      FLASH_PROGRAM(FLASH_BOOT_KEY_ADDR, &boot_key[0], (FLASH_BOOT_KEY_NWORDS * 4));
  }

  // Count bits set to 1 and find the first bit set at 1 to know which word to update
  for (int i = 0; i < FLASH_BOOT_KEY_NWORDS; i++)
  {
    // Count the number of bits set
    bits_set = bits_set + __builtin_popcountl(boot_key[i]);

    // Find the first bit set to 1. If word = 0x0, __builtin_ffsl() returns zero.
    bit_index = __builtin_ffsl(boot_key[i]);

    // Save the word index and the bit_index
    if (bit_index > max_bit_index) {
      max_bit_index = bit_index;
      word_index = i;
    }
  }

  // BOOT KEY is already set up
  if ((bits_set % 2) == bank)
    return SUCCESS;

  // Update the word at bit index
  if (max_bit_index == 32)
      boot_key[word_index] = 0x0;
  else
      boot_key[word_index] = (uint32_t)(0xFFFFFFFF << max_bit_index);

  addr = FLASH_BOOT_KEY_ADDR + (word_index * 4) - FLASH_BASE_ADDR;
  DEBUG("[flash] Updated word = %08lX, address = %ld, bit_index = %d\n", boot_key[word_index], addr, max_bit_index);
  ret = aps_flash_program(addr, &boot_key[word_index], 1);

  return ret;
}


/* Return the bank from which we're executing the current code */
int FLASH_GET_CURRENT_BANK(void)
{
  int ret = ERROR;
  uint32_t cur_addr;
  cur_addr = ( uint32_t ) &FLASH_GET_CURRENT_BANK;

  /* Address error */
  if ((cur_addr < FLASH_BANK0_ADDR) || (cur_addr > FLASH_CFG_CAL_START_ADDR))
    return ret;

  if (cur_addr < FLASH_BANK1_ADDR)
    ret = 0; //bank 0
  else
    ret = 1; //bank 1

  DEBUG("[flash] Currently inside bank%d\n", ret);
  return ret;
}


/** @}*/
