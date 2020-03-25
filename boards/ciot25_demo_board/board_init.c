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

#include "periph_cpu.h"

#include "machine/pads_sel.h"
#include "machine/analog_ctl.h"
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

void platform_init(void)
{
    //__gpio_init();

#ifdef USE_SX127X
    // configure the interrupt pins here, since hw_gpio_configure_pin() is MCU
    // specific and not part of the common HAL API
    hw_gpio_configure_pin(SX127x_DIO0_PIN, true, gpioModeInput, 0);
    hw_gpio_configure_pin(SX127x_DIO1_PIN, true, gpioModeInput, 0);
#endif

#ifdef USE_CIOT25
    gpio1->dir = 0xFFFFFFFF; // all the GPIO pins are used as outputs
    gpio2->dir = 0xFFFFFFFF; // all the GPIO pins are used as outputs
#endif


/* Pads | Func0(def) |    F1     |    F2     |    F3     |    F4    |    F5     |    F6     |
 *______|____________|___________|___________|___________|__________|___________|___________|
 *   0  |        tdi |  uart1_rx | spi2_miso |  uart2_rx |  gpio1_0 |           |           |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *   1  |        tdo |  uart1_tx | spi2_miso |  uart2_tx |  gpio1_1 |           |           |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *   2  |        tck | uart1_rts |  spi2_sck |     cap1a |    pwm1a |           |   gpio1_2 |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *   3  |        tms | uart1_cts |  spi2_ssn |   spi2_en |    cap2a |     pwm2a |   gpio1_3 |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *   4  |    gpio1_4 | spi1_miso |     pwm1a | spi2_miso |  i2c_scl |  uart1_rx |           |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *   5  |    gpio1_5 | spi1_mosi |           | spi2_mosi |  i2c_sda |  uart1_tx |           |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *   6  |    gpio1_6 |  spi1_sck |     pwm2a |  spi2_sck |          | uart1_rts |           |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *   7  |    gpio1_7 |  spi1_ssn |  uart2_rx |  spi2_ssn |  spi2_en | uart1_cts |           |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *   8  |    gpio1_8 |   spi1_en |  uart2_tx |     cap1a |    pwm1a |           |           |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *   9  |    gpio1_9 |   i2c_scl |     cap1a |     pwm1a |    pwm2a |  uart2_rx | modem_clk |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *  10  |   gpio1_10 |   i2c_sda |     cap2a |     cap1a |    pwm1a |  uart2_tx |modem_data |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 *  11  |    gpio2_0 |   gpio2_0 |   gpio2_0 |   gpio2_0 |  gpio2_0 |   gpio2_0 |   gpio2_0 |
 *------|------------|-----------|-----------|-----------|----------|-----------|-----------|
 */
    /* Default configuration for pads_sel */
    pads_sel->pad4_sel = 1;
    pads_sel->pad5_sel = 1;
    pads_sel->pad6_sel = 1;
    pads_sel->pad7_sel = 1;
    pads_sel->pad8_sel = 0;
    pads_sel->pad9_sel = 5;
    pads_sel->pad10_sel = 5;

#ifdef USE_EPD_SSD1606
    pads_sel->pad4_sel = 1;
    pads_sel->pad5_sel = 1;
    pads_sel->pad6_sel = 1;
    pads_sel->pad7_sel = 0;
    pads_sel->pad8_sel = 0;
    pads_sel->pad9_sel = 0;
    pads_sel->pad10_sel = 5;
#endif
#ifdef USE_DHT22
    pads_sel->pad4_sel = 0;
    pads_sel->pad5_sel = 1;
    pads_sel->pad6_sel = 1;
    pads_sel->pad7_sel = 1;
    pads_sel->pad8_sel = 0;
    pads_sel->pad9_sel = 0;
    pads_sel->pad10_sel = 5;
#endif

//    __led_init();

    // This activates the 0.65volt reference for ALL the LDOs apart from the
    // LDO always on before activating the LDOs this signal must be active
    analog_ctl-> ldo_vref_en = 1;

    // DIGITAL SWITCH DOMAIN (CPU XCR SUBSYSTEM, etc ...)
    analog_ctl-> dig_ldo2_en = 1; // Internal for V1 (1.2V). V2 ?
    analog_ctl-> dig_ldo2_iload = 1;
    //analog_ctl-> dig_ldo2_trim = 0x07;

    // MASH LDO
    analog_ctl-> pll_dig_ldo2_en = 0; // Internal for V1 but 1.16V instead of 1.2V. V2 ?
    analog_ctl-> pll_dig_ldo2_ovr_en = 0;
    analog_ctl-> pll_dig_ldo2_iload = 1;
    analog_ctl-> pll_dig_ldo2_trim = 0x7;

    // PLL_PFCD LDO
    analog_ctl-> pll_pfdcp_ldo2_en = 1; // Internal for V1 but 1.16V instead of 1.2V. V2 ?
    analog_ctl-> pll_pfdcp_ldo2_ovr_en = 0;
    analog_ctl-> pll_pfdcp_ldo2_iload = 1;
    analog_ctl-> pll_pfdcp_ldo2_trim = 0x7;

    // PLL_VCO LDO
    analog_ctl-> pll_vco_ldo2_en = 0; // External for V1 (1,5V). V2? /////
    analog_ctl-> pll_vco_ldo2_ovr_en = 0;
    analog_ctl-> pll_vco_ldo2_iload = 1;
    analog_ctl-> pll_vco_ldo2_trim = 0x7;

    // PLL_DIVIDER LDO
    analog_ctl-> rxtx_ldo2_en = 0; // External for V1 (1,5V). V2? ///////
    analog_ctl-> rxtx_ldo2_ovr_en = 0;
    analog_ctl-> rxtx_ldo2_iload = 1;                              //////
    analog_ctl-> rxtx_ldo2_trim = 0x7;

    // LDO3 TX2V (for transmission power)
    analog_ctl-> ldo3_en = 1; // External for V1 (it use to much current). V2?
    analog_ctl-> ldo3_iload = 1;
    analog_ctl-> ldo3_trim = 0x7;
    analog_ctl-> ldo3_bypass = 0;

    // 32 MHz crystal oscillator
    analog_ctl-> xtal_clk_adet_en = 1;
    analog_ctl-> xtal_clk_buffer_en = 1;
    analog_ctl-> xtal_clk_c1_bank = 105;
    analog_ctl-> xtal_clk_c2_bank = 105;
    analog_ctl-> xtal_clk_core_en = 1;
    analog_ctl-> xtal_clk_en = 1;
    analog_ctl-> xtal_clk_itrim = 0;

    // ADC LDO
    analog_ctl-> rxif_ldo2_en = 1; // Internal for V1 but 1.16V instead of 1.2V. V2 ?
    analog_ctl-> rxif_ldo2_ovr_en = 0;
    analog_ctl-> rxif_ldo2_iload = 0;
    analog_ctl-> rxif_ldo2_trim = 0x7;

    // Some ADC config (ex: RX_ADC__CLK_PHASE)
    analog_ctl-> tbd_o =  ANALOG_RXFE_DC_TRIM | (ANALOG_RX_ADC__I_INV << 2) |
                          (ANALOG_RX_ADC__Q_INV << 3) | (ANALOG_RX_ADC__SW_IQ << 4) |
                          (ANALOG_RX_ADC__CLK_PHASE << 5) | (ANALOG_TEST_CTRL33_6 << 6);

    //__watchdog_init();
}



void board_init(void)
{
    /* first initialize STDIO over UART (for early printf...)
     * !Be warned about xtimer initialization conflict in tx_isr mode!
     */
    stdio_init();

    platform_init();

    /* initialize the CPU */
    cpu_init();

    //__watchdog_init(); // done in periph_init ??
    aps_fs_init();
}
