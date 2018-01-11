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
 * @brief       CORTUS peripheral configuration
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 */

#ifndef PERIPH_CONF_H
#define PERIPH_CONF_H

#include "periph_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Clock configuration
 * @{
 */
/* the main clock is fixed to 32MHZ */
#define CLOCK_CORECLOCK     (32000000U)
/** @} */

static inline uint32_t clk_source(uint8_t clk)
{
    switch(clk)
    {
    case 0:
        return 50000000;
    case 1:
        return 25000000;
    case 2:
        return 12500000;
    case 4:
    default:
        return 3125000;
    }
}

/**
 * @name Timer peripheral configuration
 * @{
 */

static const timer_conf_t timer_config[] = {
    {
        .dev      = (Timer *)SFRADR_TIMER1,
        .max      = 0xffffffff,
        .clk      = 0, // 50 MHz
        .cmpb[0] = (Timer_Cmp*)SFRADR_TIMER1_CMPA,
        .cmpb[1] = (Timer_Cmp*)SFRADR_TIMER1_CMPB,
        .irqn[0] = IRQ_TIMER1_CMPA,
        .irqn[1] = IRQ_TIMER1_CMPB,
        .capb = (Timer_Cap*)SFRADR_TIMER1_CAPA
    }
};

#define TIMER_NUMOF         (sizeof(timer_config) / sizeof(timer_config[0]))

/**
 * @name UART configuration
 * @{
 */
#define UART_NUMOF          (1U)
/* UART pin configuration */

/** @} */

/**
 * @name   ADC configuration
 *
 * The configuration consists simply of a list of channels that should be used
 * @{
 */
#define ADC_NUMOF          (1U)
/** @} */

/**
 * @name    Led definition
 * @{
 */
#define LED_NUMOF          (8U)
/** @} */

/**
 * @name    Console configuration
 * @{
 */
#define CONSOLE_UART        0
#define CONSOLE_LOCATION    1
#define CONSOLE_BAUDRATE    115200
/** @} */

/**
 * @name    User button definitions
 * @{
 */
#define USERBUTTONS_NUMOF  2
#define BUTTON0            GPIO_PIN(gpioPortA, 4)
#define BUTTON1            GPIO_PIN(gpioPortA, 5)
/** @} */


 /**
 * @name    SPI configuration
 * @{
 */
static const uint16_t spi_divtable[5] = {
        /* for clock input @ 50MHz */
        499,  /* -> 100 KHz */
        124,  /* -> 400 KHz */
        49,   /* -> 1 MHz */
        9,    /* -> 5 MHz */
        4     /* -> 10 MHz */
};

static const spi_conf_t spi_config[] = {
    {
        .dev   = (SPI *)SFRADR_SPI,
        .cs_pin = GPIO_PIN(gpioPortA, 2),
    },
    /*{
        .dev   = (SPI *)SFRADR_SPI2,
        .cs_pin = GPIO_PIN(gpioPortA, 2),
    }*/
};
#define SPI_NUMOF           (1U)

//#define SPI_PIN_MISO        GPIO_PIN(P3,2)
//#define SPI_PIN_MOSI        GPIO_PIN(P3,1)
//#define SPI_PIN_CLK         GPIO_PIN(P3,3)
/* SPI configuration */


/** @} */

/**
 * @name    CC1101 PIN definitions
 * @{
 */
#ifdef USE_CC1101
#define CC1101_SPI_USART    1  // not used
#define CC1101_SPI_BAUDRATE 8  // divider
#define CC1101_SPI_PIN_CS   GPIO_PIN(gpioPortA, 2)
#define CC1101_GDO0_PIN     GPIO_PIN(gpioPortA, 0)
#define CC1101_GDO2_PIN     GPIO_PIN(gpioPortA, 1)
#endif
/** @} */

/**
 * @name    Sx127x PIN definitions
 * @{
 */
#ifdef USE_SX127X

#define SX127x_SPI_INDEX    1
#define SX127x_SPI_PIN_CS   GPIO_PIN(gpioPortA, 2)
#define SX127x_SPI_BAUDRATE 8 //10000000
#define SX127x_DIO0_PIN     GPIO_PIN(gpioPortA, 0)
#define SX127x_DIO1_PIN     GPIO_PIN(gpioPortA, 1)
#endif
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CONF_H */
