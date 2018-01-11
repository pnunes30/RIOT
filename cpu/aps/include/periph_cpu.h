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

#include "cpu.h"
#include "machine/spi.h"
#include "machine/timer.h"
#include "machine/timer_cap.h"
#include "machine/timer_cmp.h"
#include "machine/uart.h"
#include "machine/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

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
#define GPIO_PIN(port, pin)      (((uint32_t)port) | pin)

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
 * @brief   Length of the CPU_ID in octets
 */
#define CPUID_LEN           (16U)

/**
 * @brief   All APS timers are 32-bit wide
 */
#define TIMER_MAX_VAL       (0xffffffff)

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
    Timer *dev;                   /**< timer device */
    uint32_t max;                 /**< maximum value to count to (16/32 bit) */
    uint8_t clk;                  /**< timer clock source 0 = 50MHZ / 1 = 25 MHz / 2 = 12.5MHz / 3 = 3.125MHZ*/
    Timer_Cmp *cmpb[TIMER_CHAN];  /**< timer compare block */
    uint8_t irqn[TIMER_CHAN];     /**< interrupt number of each compare block */
    Timer_Cap *capb;              /**< timer capture block */
} timer_conf_t;

/**
 * @brief   UART configuration data
 */
// no need of this structure since only one UART is available

typedef struct {
    Uart *dev;              /**< U(S)ART device used */
    uint8_t irqn;           /**< interrupt number of the device */
} uart_conf_t;

/**
 * @brief   SPI configuration data
 */
typedef struct {
    SPI *dev;               /**< SPI module to use */
    gpio_t cs_pin;          /**< HWCS pin, set to GPIO_UNDEF if not mapped */
} spi_conf_t;

/**
 * @brief   Configure the given GPIO pin to be used with the given MUX setting
 *
 * @param[in] pin           GPIO pin to configure
 * @param[in] mux           MUX setting to use
 */
//void gpio_init_mux(gpio_t pin, gpio_mux_t mux);

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CPU_H */
/** @} */
