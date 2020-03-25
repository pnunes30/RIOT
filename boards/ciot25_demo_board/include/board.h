/*
 * Copyright (C) 2017 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     boards_aps
 * @{
 *
 * @file
 * @brief       Board specific definitions for the Cortus_fpga evaluation board.
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 */

#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#include "periph_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Define the CPU model
 */
// TODO


#if defined(MODULE_SX1276)
/**
 * @name    sx1276 configuration
 * @{
 */
#define SX127X_PARAM_SPI                    (SPI_DEV(0))
#define SX127X_PARAM_DIO0                   GPIO_PIN(gpioPortA, 0)
#define SX127X_PARAM_DIO1                   GPIO_PIN(gpioPortA, 1)
#define SX127X_PARAM_SPI_NSS                GPIO_PIN(gpioPortA, 2)

//RESET, DI02 and DI03 are not mapped
//#define SX127X_PARAM_RESET                GPIO_PIN(gpioPortA, 3)
//#define SX127X_PARAM_DIO2                 GPIO_PIN(gpioPortA, 4)
//#define SX127X_PARAM_DIO3                 GPIO_PIN(gpioPortA, 5)
#endif


#if defined(MODULE_DHT)
/**
 * @name    DHT configuration
 * @{
 */
#define DHT_PARAM_PIN               GPIO_PIN(gpioPortA, 3)
#define DHT_PARAM_TYPE              (DHT22)
#define DHT_PARAM_PULL              (GPIO_IN)

#define DHT_PARAMS                  { .pin     = DHT_PARAM_PIN,  \
                                      .type    = DHT_PARAM_TYPE, \
                                      .in_mode = DHT_PARAM_PULL }
#endif

/**
 * @name Macros for controlling the on-board LEDs.
 * @{
 */
#if 0 /* NO Leds on CioT25*/
#define LED0_PIN            GPIO_PIN(SFRADR_GPIO_EDGE1,16)
#define LED1_PIN            GPIO_PIN(SFRADR_GPIO_EDGE1,17)
#define LED2_PIN            GPIO_PIN(SFRADR_GPIO_EDGE1,18)
#define LED3_PIN            GPIO_PIN(SFRADR_GPIO_EDGE1,19)
#define LED4_PIN            GPIO_PIN(SFRADR_GPIO_EDGE1,20)
#define LED5_PIN            GPIO_PIN(SFRADR_GPIO_EDGE1,21)
#define LED6_PIN            GPIO_PIN(SFRADR_GPIO_EDGE1,22)
#define LED7_PIN            GPIO_PIN(SFRADR_GPIO_EDGE1,23)

#define LED0_ON             gpio_set(LED0_PIN)
#define LED0_OFF            gpio_clear(LED0_PIN)
#define LED0_TOGGLE         gpio_toggle(LED0_PIN)

#define LED1_ON             gpio_set(LED1_PIN)
#define LED1_OFF            gpio_clear(LED1_PIN)
#define LED1_TOGGLE         gpio_toggle(LED1_PIN)
#endif
/** @} */

/**
 * @brief User button
 * @{
 */
//#define BTN0_PIN            GPIO_PIN(PORT_A, 0)
//#define BTN0_MODE           GPIO_IN
/** @} */


#if defined(MODULE_MTD) || DOXYGEN

#if defined(USE_PERIPH_CORTUS_MTD_FPGA_M25P32)
/* 0-0x280000: fpga bitmap  (sectors #0->#39)
 * 0x280000-0x280004 : size of bin image
 * => 0x280000+4*SECTOR_SIZE: available for extra data.  */
#define APS_FPGA_M25P32_DEV               SPI_DEV(1)     /**< index in spi_config[] */
#define APS_FPGA_M25P32_CLK               SPI_CLK_10MHZ  /* M25P32 max freq is 75MHz */
#define APS_FPGA_M25P32_CS                SPI_CS_UNDEF   /**< Flash CS pin is mapped on SPI->clk_en, no gpio*/
#define APS_FPGA_M25P32_MODE              SPI_MODE_0
#define APS_FPGA_M25P32_PAGE_SIZE         256
#define APS_FPGA_M25P32_PAGES_PER_SECTOR  256
#define APS_FPGA_M25P32_SECTOR_COUNT      64
#define APS_FPGA_M25P32_BASE_SECTOR       (40+4)        /**< Sectors [0..39] fpga bitmap, [40(+4)..63] (bin+) free. 4*64K reserved for bin image */
#define APS_FPGA_M25P32_BASE_ADDR         (0x280000+4*0x10000)
#if (APS_FPGA_M25P32_BASE_SECTOR*APS_FPGA_M25P32_PAGE_SIZE*APS_FPGA_M25P32_PAGES_PER_SECTOR != APS_FPGA_M25P32_BASE_ADDR)
#error
#endif
#else
/**
 * @name    MTD emulation configuration
 * @{
 */
/* Test mock object implementing a simple RAM-based mtd */

#ifndef SECTOR_COUNT
#define SECTOR_COUNT 64
#endif
#ifndef PAGE_PER_SECTOR
#define PAGE_PER_SECTOR 8
#endif
#ifndef PAGE_SIZE
#define PAGE_SIZE 128
#endif
/** @} */

#endif /*dummy|fpga*/

/** mtd flash emulation device */
extern mtd_dev_t * const mtd0;
extern mtd_dev_t * const mtd1;
extern mtd_dev_t * const mtd2;
extern mtd_dev_t * const mtd_vfs;

/** Default MTD device */
#define MTD_0 mtd0
#define MTD_1 mtd1
#define MTD_2 mtd2
#define MTD_VFS mtd_vfs

#endif


////////////////////////////////////////////////////////////////////////////////
/**
 * @name    D7A FS config
 * @{
 */

#if defined(MODULE_D7A) //D7A
#if defined(MODULE_VFS)
#ifndef MODULE_TMPFS
#error "tmpfs required with vfs module"
#endif
extern vfs_mount_t aps_tmpfs_mount;
extern vfs_mount_t aps_mtd0_mount;
/* no d7 code dependency here */
#define ARCH_VFS_STORAGE_TRANSIENT    aps_tmpfs_mount
#define ARCH_VFS_STORAGE_VOLATILE     aps_tmpfs_mount
#define ARCH_VFS_STORAGE_RESTORABLE   aps_mtd0_mount
#define ARCH_VFS_STORAGE_PERMANENT    aps_mtd0_mount
#else
/*
extern blockdevice_t * const permanent_blockdevice;
extern blockdevice_t * const volatile_blockdevice;
*/
/** Platform BD drivers*/
/*
#define PLATFORM_PERMANENT_BD permanent_blockdevice
#define PLATFORM_VOLATILE_BD volatile_blockdevice
*/
#endif /*MODULE_VFS*/
#endif /*MODULE_D7A*/
/** @} */

#define STDIO_UART_DEV       (UART_DEV(1))

/**
 * @brief Initialize board specific hardware, including clock, LEDs and std-IO
 */
void board_init(void);


#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
/** @} */
