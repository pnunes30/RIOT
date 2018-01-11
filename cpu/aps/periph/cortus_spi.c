/*
 * Copyright (C) 2017 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @ingroup     drivers_periph_spi
 * @{
 *
 * @file
 * @brief       Low-level SPI driver implementation
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @}
 */

#include "cpu.h"
#include "mutex.h"
#include "assert.h"
#include "periph/spi.h"

#include "machine/spi.h"

static mutex_t locks[SPI_NUMOF];

static inline SPI *dev(spi_t bus)
{
    return spi_config[bus].dev;
}

void spi_init(spi_t bus)
{
    assert(bus < SPI_NUMOF);

    /* initialize device lock */
    mutex_init(&locks[bus]);

    /* trigger pin initialization */
    spi_init_pins(bus);

    /* configure SPI */
    dev(bus)->master = 1; // master mode
    dev(bus)->mode_fault = 0; //ignore faults on spi bus

    /* disable SPI module */
    dev(bus)->clk_en = 0;

    // don't use interrupt for SPI
    //irq[IRQ_SPI].ipl = 0;
    //irq[IRQ_SPI].ien = 0;
}

void spi_init_pins(spi_t bus)
{
// no specific initialisation required for APS cpu
}

//int spi_init_cs(spi_t bus, spi_cs_t cs)
//implemented in the generic SPI functions

int spi_acquire(spi_t bus, spi_cs_t cs, spi_mode_t mode, spi_clk_t clk)
{
    /* lock bus */
    mutex_lock(&locks[bus]);
    /* enable SPI device clock */
    dev(bus)->clk_en = 1;

    dev(bus)->config = mode | 0x0; // MSB first, unidirectional
    dev(bus)->selclk = 0; // 0:50MHz, 1:25MHz, 2:12.5MHZ, 3:3.125MHz

    /* set Clock Rate Divider */
    dev(bus)->divider = spi_divtable[clk]; // divtable is computed according clock input set to 50MHz

    return SPI_OK;
}

void spi_release(spi_t bus)
{
    /* disable device and release lock */
    while ((dev(bus)->bus_active & SPI_BUS_ACTIVE)) {}
    dev(bus)->clk_en = 0;

    mutex_unlock(&locks[bus]);
}

void spi_transfer_bytes(spi_t bus, spi_cs_t cs, bool cont,
                        const void *out, void *in, size_t len)
{
    const uint8_t *outbuf = out;
    uint8_t *inbuf = in;

    /* make sure at least one input or one output buffer is given */
    assert(outbuf || inbuf);

    /* we need to recast the data register to uint_8 to force 8-bit access */
    volatile uint8_t *DR = (volatile uint8_t*)&(dev(bus)->DR);

    /* active the given chip select line */
    if (cs != SPI_CS_UNDEF) {
        gpio_clear((gpio_t)cs);
    }

    /* transfer data, use shortpath if only sending data */
    if (!inbuf) {
        for (size_t i = 0; i < len; i++) {
            while (!(dev(bus)->tx_status & SPI_TX_SR_FIFO_NOT_FULL)) {} // wait until tx fifo is available
            dev(bus)->tx_data = outbuf[i];
        }
        /* wait until everything is finished and empty the receive buffer */
        while ((dev(bus)->bus_active & SPI_BUS_ACTIVE)) {}
        while ((dev(bus)->rx_status & SPI_RX_SR_DATA_AVAILABLE)) {
            (uint8_t)dev(bus)->rx_data;
        }
    }
    else if (!outbuf) {
        for (size_t i = 0; i < len; i++) {
            while (!(dev(bus)->tx_status & SPI_TX_SR_FIFO_NOT_FULL)) {} // wait until tx fifo is available
            dev(bus)->tx_data = 0;
            while (!(dev(bus)->rx_status & SPI_RX_SR_DATA_AVAILABLE)) {} // wait until rx fifo is available
            inbuf[i] = (uint8_t)dev(bus)->rx_data;;
        }
    }
    else {
        for (size_t i = 0; i < len; i++) {
            while (!(dev(bus)->tx_status & SPI_TX_SR_FIFO_NOT_FULL)) {} // wait until tx fifo is available
            dev(bus)->tx_data = outbuf[i];
            while (!(dev(bus)->rx_status & SPI_RX_SR_DATA_AVAILABLE)) {} // wait until rx fifo is available
            inbuf[i] = (uint8_t)dev(bus)->rx_data;
        }
    }

    /* make sure the transfer is completed before continuing */
    while ((dev(bus)->bus_active & SPI_BUS_ACTIVE)) {}

    /* release the chip select if not specified differently */
    if ((!cont) && (cs != SPI_CS_UNDEF)) {
            gpio_set((gpio_t)cs);
    }
}
