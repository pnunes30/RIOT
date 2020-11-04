/*
 * Copyright (C) 2017 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @{
 */

/**
 * @file
 * @brief       CPU specific definitions for internal peripheral handling
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 */

#ifndef PERIPH_CPU_H
#define PERIPH_CPU_H

/* enable features here instead of makefile (eclipse nav)*/
#define USE_PERIPH_CORTUS_PM
#define USE_PERIPH_CORTUS_UART
#define USE_PERIPH_CORTUS_TIMER
#define USE_PERIPH_CORTUS_COUNTER
#define USE_PERIPH_CORTUS_GPIO
#define USE_PERIPH_CORTUS_SPI
#define USE_PERIPH_CORTUS_RTT
//#define USE_PERIPH_CORTUS_WATCHDOG

// M25P32 must be explicitly enabled in project Makefile (double check)
// MODULE_SPI_NOR requires it, see cortus_mtd.c
#ifdef __ECLIPSE__
#define USE_PERIPH_CORTUS_MTD_FPGA_M25P32
#endif


#include "cpu.h"
/* explicit Bsp/ in include path to avoid toolchain includes */
#include "Bsp/machine/sfradr.h"
#include "Bsp/machine/cpu.h"
#include "Bsp/machine/ic.h"
#include "Bsp/machine/flash_ctl.h"

#ifdef USE_PERIPH_CORTUS_UART
#include "Bsp/machine/uart.h"
#endif

#if defined(USE_PERIPH_CORTUS_TIMER) || defined(USE_PERIPH_CORTUS_RTT)
#include "Bsp/machine/timer.h"
#include "Bsp/machine/timer_cap.h"
#include "Bsp/machine/timer_cmp.h"
#endif

#ifdef USE_PERIPH_CORTUS_COUNTER
#include "Bsp/machine/counter.h"
#endif

#ifdef USE_PERIPH_CORTUS_GPIO
#include "Bsp/machine/gpio.h"
#endif

#ifdef USE_PERIPH_CORTUS_SPI
#include "Bsp/machine/spi.h"
#endif

#ifdef USE_PERIPH_CORTUS_WATCHDOG
#include "Bsp/machine/wdt.h"
#endif

#ifdef USE_PERIPH_CORTUS_I2C
#include "Bsp/machine/i2c.h"
#endif

#if defined(MODULE_MTD)
#include "mtd.h"
#endif

#if defined(MODULE_XCVR)
#include "Bsp/machine/xcvr.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Available clock input
 */
enum {
    CLOCK_INPUT0,
    CLOCK_INPUT1,
    CLOCK_INPUT2,
    CLOCK_INPUT3
};

/**
 * @brief Overwrite the default gpio_t type definition
 */
#define HAVE_GPIO_T
typedef uint32_t gpio_t;
/** @} */

/**
 * @brief Definition of a fitting UNDEF value
 */
#define GPIO_UNDEF          (0xffffffff)

/**
 * @brief Define a CPU specific GPIO pin generator macro
 */
/* Only 8 pins per gpio block on Ciot25 */
#define GPIO_PIN_NUMOF		     (8)				/*powerof2*/
#define GPIO_PIN_MASK		     (GPIO_PIN_NUMOF-1)

#define GPIO_PIN(port, pin_id)   (((uint32_t)port) | pin_id)
#define GPIO_PORT(pin)		     ((Gpio *)(pin & ~(GPIO_PIN_MASK)))
#define GPIO_PIN_ID(pin)	     (pin & GPIO_PIN_MASK)
#define GPIO_IRQ(port)			 (((uint32_t)port==SFRADR_GPIO2) ? IRQ_GPIO2 : IRQ_GPIO1)	/*ugly*/

/**
 * @brief Declare needed generic SPI functions
 * @{
 */
#define PERIPH_SPI_NEEDS_INIT_CS
#define PERIPH_SPI_NEEDS_TRANSFER_BYTE
#define PERIPH_SPI_NEEDS_TRANSFER_REG
#define PERIPH_SPI_NEEDS_TRANSFER_REGS
/** @} */

/**
 * @brief   Length of the CPU_ID in octets. Used to generate UUID...
 */
#define CPUID_LEN           (32U)

/**
 * @brief   Prevent shared periph_common timer functions from being used
 */
#define PERIPH_TIMER_PROVIDES_SET

/**
 * @brief   All APS timers are 32-bit wide
 */
#define TIMER_WIDTH			(32)
#define TIMER_MAX_VAL       (0xffffffff>>(32-TIMER_WIDTH))

/**
 * @brief   All APS timers have 2 capture-compare channels (CMPA/CMPB)
 */
#define TIMER_CHAN          (2U)

/**
 * @name    ADC configuration, valid for all boards using this CPU
 *
 * The APS has a fixed mapping of ADC pins and a fixed number of ADC channels,
 * so this ADC configuration is valid for all boards using this CPU. No need for
 * any board specific configuration.
 */
#define ADC_NUMOF           (1U)

#ifndef DOXYGEN
/**
 * @brief   Override GPIO modes
 * @{
 */
#define HAVE_GPIO_MODE_T
typedef enum {
    GPIO_IN    = 0,    /**< IN */
    GPIO_IN_PD = 0xf,  /**< not supported by HW */
    GPIO_IN_PU = 0xf,  /**< IN with pull-up not supported by HW */
    GPIO_OUT   = 1,    /**< OUT (push-pull) */
    GPIO_OD    = 0xf,  /**< OD not supported by HW */
    GPIO_OD_PU = 0xf,  /**< OD with pull-up not supported by HW */
} gpio_mode_t;
/** @} */

/**
 * @brief Override flank configuration values
 * @{
 */
#define HAVE_GPIO_FLANK_T
typedef enum {
    GPIO_RISING = 1,        /**< emit interrupt on rising flank */
    GPIO_FALLING = 2,       /**< emit interrupt on falling flank */
    GPIO_BOTH = 3           /**< emit interrupt on both flanks */
} gpio_flank_t;
/** @} */

#endif /* ndef DOXYGEN */

/**
 * @brief   Override default SPI modes
 * @{
 */
#define HAVE_SPI_MODE_T
typedef enum {
    SPI_MODE_0 = 0,         /**< CPOL=0, CPHA=0 */
    SPI_MODE_1 = 2,         /**< CPOL=0, CPHA=1 */
    SPI_MODE_2 = 1,         /**< CPOL=1, CPHA=0 */
    SPI_MODE_3 = 3          /**< CPOL=1, CPHA=1 */
} spi_mode_t;
/** @} */

/**
 * @name   Override SPI clock settings
 * @{
 */
#define HAVE_SPI_CLK_T
typedef enum {
	SPI_CLK_100KHZ =   0, /* see div_table */
	SPI_CLK_400KHZ =   1, /* see div_table */
	SPI_CLK_1MHZ   =   2, /* see div_table */
	SPI_CLK_5MHZ   =   3, /* see div_table */
	SPI_CLK_10MHZ  =   4  /* see div_table */
} spi_clk_t;


/**
 * @brief   Override ADC resolution values
 * @{
 */
#define HAVE_ADC_RES_T
typedef enum {
    ADC_RES_6BIT  = 0,                    /**< not applicable */
    ADC_RES_8BIT  = 0,                    /**< not applicable */
    ADC_RES_10BIT = 0,                    /**< not applicable */
    ADC_RES_12BIT = 1,                    /**< ADC resolution: 12 bit */
    ADC_RES_14BIT = 0,                    /**< not applicable */
    ADC_RES_16BIT = 0                     /**< not applicable */
} adc_res_t;
/** @} */

/**
 * @brief   Timer configuration data
 */
typedef struct {
    Timer *    dev;                 /**< timer device */
    uint8_t    clk;                 /**< timer clock source 0 = 50MHZ / 1 = 25 MHz / 2 = 12.5MHz / 3 = 3.125MHZ*/
    uint8_t    pre;		            /**< timer prescaler (0-3), freq divider = 2^pre (1;2;4;8) */
    uint32_t   max;                 /**< maximum value to count to (16/32 bit) = mult * (1<<XTIMER_WIDTH);*/
    uint8_t    irqo;                /**< irq number of main block for overflow isr */
    Timer_Cmp *cmp[TIMER_CHAN];     /**< timer compare block */
    uint8_t    irqn[TIMER_CHAN];    /**< interrupt number of each compare block */
    Timer_Cap *cap;                 /**< timer capture block */
} timer_conf_t;

/**
 * @brief   UART configuration data
 */
// no need of this structure since only one UART is available

typedef struct {
    Uart *dev;              /**< U(S)ART device used */
    uint8_t rx_irqn;           /**< RX interrupt number of the device */
    uint8_t rx_always_on;      /**< RX isr mode: 0= on demand (i.e. in getchar, to prevent flood); 1=always on at init */
    uint8_t tx_irqn;           /**< TX interrupt number of the device */
    int32_t tx_wait_poll;      /**< TX max poll wait: 0=no, -1=forever, >0 loop cnt */
    int32_t tx_wait_isr;       /**< TX max isr wait:  0=no, -1=forver, >0 usec timeout */
} uart_conf_t;


#ifdef USE_PERIPH_CORTUS_SPI
/**
 * @brief   SPI configuration data
 */
typedef struct {
    SPI *dev;               /**< SPI module to use */
    gpio_t cs_pin;          /**< HWCS pin, set to GPIO_UNDEF if not mapped */
} spi_conf_t;

#endif /*USE_PERIPH_CORTUS_SPI*/

/**
 * @brief   Configure the given GPIO pin to be used with the given MUX setting
 *
 * @param[in] pin           GPIO pin to configure
 * @param[in] mux           MUX setting to use
 */
//void gpio_init_mux(gpio_t pin, gpio_mux_t mux);

/**
 * @name    Power management configuration
 * @brief   Prevent shared periph_common pm functions from being used
 * @{
 */
#define PROVIDES_PM_OFF
#define PROVIDES_PM_SET_LOWEST

/* Do not overload periph_init from periph_common */
#ifdef USEMODULE_PERIPH_COMMON
#undef PROVIDES_PERIPH_INIT
#endif

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CPU_H */
/** @} */
