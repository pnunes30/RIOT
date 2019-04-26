/*********************************************************************************
 * This confidential and proprietary software may be used only as authorized 
 *                      by a licensing agreement from                           
 *                           Cortus S.A.S.
 *
 *                 (C) Copyright 2004-2019 Cortus S.A.S.
 *                        ALL RIGHTS RESERVED
 *
 * The entire notice above must be reproduced on all authorized copies
 * and any such reproduction must be pursuant to a licensing agreement 
 * from Cortus S.A.S. (http://www.cortus.com)
 *
 * $CortusRelease$
 * $FileName$
 *
 *********************************************************************************/

#ifndef PERIPH_CPU_H
#error "Do not include this bsp header directly. Please include periph_cpu.h"
#endif

#ifndef _SPI_H
#define _SPI_H
#include <machine/sfradr.h>

#define SPI_COUNT 2

typedef struct SPI
{
    /* Transmit data to tx buffer */
    volatile unsigned tx_data;

    /* Receive data from rx buffer */
    volatile unsigned rx_data;

    /* Status register */
    volatile unsigned tx_status;
    volatile unsigned rx_status;

    /* Input clock selection */
    volatile unsigned selclk;

    /* Clock divider */
    volatile unsigned divider;

    /* Internal clock enable - Sleep mode */
    volatile unsigned clk_en;

    /* SPI in master mode or slave mode */
    volatile unsigned master;

    /* Mode fault enable */
    volatile unsigned mode_fault;

    /* Configuration 
       bit       config
       4         bidirectionnal direction
       3         bidirectionnal mode enable
       2         lsb first
       1         sck phase 0 odd edges 1 even edges
       0         sck polarity
    */
    volatile unsigned config;

    /* Activity on bus - should always check before writing fifo */
    volatile unsigned bus_active;

    /* Mask register for interrupt */
    volatile unsigned tx_mask;
    volatile unsigned rx_mask;

} SPI;

/***************  Bit definition ******************/
#define APS_SPI_TX_SR_FIFO_NOT_FULL       0x01 /*!< space in the fifo - bit 0 of spi.tx_status*/
#define APS_SPI_RX_SR_DATA_AVAILABLE      0x01 /*!< data available in fifo - bit 0 of spi.rx_status*/
#define APS_SPI_BUS_ACTIVE                0x01 /*!< activity on spi bus - bit 0 of spi.bus_active*/

#ifdef __APS__
#define spi1 ((SPI *)SFRADR_SPI)
#define spi2 ((SPI *)SFRADR_SPI2)
#else
extern SPI __spi;
#define spi1 (&__spi)
#define spi2 (&__spi)
#endif

#endif
