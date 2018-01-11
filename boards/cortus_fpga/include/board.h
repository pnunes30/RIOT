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
#include "periph/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Define the CPU model
 */
// TODO

/**
 * @name    Xtimer configuration
 * @{
 */
#define XTIMER_WIDTH                (16)
/** @} */

/**
 * @name Macros for controlling the on-board LEDs.
 * @{
 */

#define LED0_PIN            GPIO_PIN(gpioPortA,16)
#define LED1_PIN            GPIO_PIN(gpioPortA,17)
#define LED2_PIN            GPIO_PIN(gpioPortA,18)
#define LED3_PIN            GPIO_PIN(gpioPortA,19)
#define LED4_PIN            GPIO_PIN(gpioPortA,20)
#define LED5_PIN            GPIO_PIN(gpioPortA,21)
#define LED6_PIN            GPIO_PIN(gpioPortA,22)
#define LED7_PIN            GPIO_PIN(gpioPortA,23)

#define LED0_ON             gpio_set(LED0_PIN)
#define LED0_OFF            gpio_clear(LED0_PIN)
#define LED0_TOGGLE         gpio_toggle(LED0_PIN)

#define LED1_ON             gpio_set(LED1_PIN)
#define LED1_OFF            gpio_clear(LED1_PIN)
#define LED1_TOGGLE         gpio_toggle(LED1_PIN)

/** @} */

/**
 * @brief User button
 * @{
 */
//#define BTN0_PIN            GPIO_PIN(PORT_A, 0)
//#define BTN0_MODE           GPIO_IN
/** @} */

/**
 * @brief Initialize board specific hardware, including clock, LEDs and std-IO
 */
void board_init(void);


#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
/** @} */
