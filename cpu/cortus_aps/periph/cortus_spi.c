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

#define ENABLE_DEBUG    (0)
#include "debug.h"

#ifdef USE_PERIPH_CORTUS_SPI

static mutex_t locks[SPI_NUMOF];

static inline SPI *dev(spi_t bus)
{
    return spi_config[bus].dev;
}

/* FIXME: bus_active check (cf. cortus peripherals manual):
 *  the registers must not be modified while bus active, but the
 *  _OPTIONAL macro can be disabled here as it is invoked at moments
 *  when the spi bus is *SUPPOSED* to be inactive.
 *  Reenable it if suspicious behaviour detected. TBC.
 */
#define WAIT_SPI_IDLE(_spi)            while ((_spi->bus_active & APS_SPI_BUS_ACTIVE)) {}
#define WAIT_SPI_IDLE_OPTIONAL(_spi)   //WAIT_SPI_IDLE(_spi)

void spi_init(spi_t bus)
{
    assert(bus < SPI_NUMOF);
    (void)bus;

    /* initialize device lock */
    mutex_init(&locks[bus]);

    /* trigger pin initialization */
    spi_init_pins(bus);

    /* disable SPI module */
    dev(bus)->clk_en = 0;
    WAIT_SPI_IDLE(dev(bus));

    /* configure SPI */
    dev(bus)->master = 1; // master mode
    WAIT_SPI_IDLE_OPTIONAL(dev(bus));
    dev(bus)->mode_fault = 0; //ignore faults on spi bus
    WAIT_SPI_IDLE_OPTIONAL(dev(bus));

    // don't use interrupt for SPI
    //irq[IRQ_SPI].ipl = 0;
    //irq[IRQ_SPI].ien = 0;
}

void spi_init_pins(spi_t bus)
{
	(void)bus;
	// no specific initialisation required for APS cpu
}

//int spi_init_cs(spi_t bus, spi_cs_t cs)
//implemented in the generic SPI functions

int spi_acquire(spi_t bus, spi_cs_t cs, spi_mode_t mode, spi_clk_t clk)
{
	DEBUG("spi_acquire(%d, 0x%x, 0x%x, 0x%x)\n", bus, (unsigned int)cs, mode, clk);
    /* lock bus */
    mutex_lock(&locks[bus]);
    /* enable SPI device clock */

    dev(bus)->config = mode | 0x0; // MSB first, unidirectional
    WAIT_SPI_IDLE_OPTIONAL(dev(bus));
    dev(bus)->selclk = SPI_SELCLK;
    WAIT_SPI_IDLE_OPTIONAL(dev(bus));

    /* set Clock Rate Divider */
    dev(bus)->divider = spi_divtable[clk]; // divtable based on sys clock_frequency
    WAIT_SPI_IDLE_OPTIONAL(dev(bus));

    dev(bus)->clk_en = 1;
    return SPI_OK;
}

void spi_release(spi_t bus)
{
	/* disable device and release lock */
    WAIT_SPI_IDLE(dev(bus));
    dev(bus)->clk_en = 0;

    mutex_unlock(&locks[bus]);
    DEBUG("spi_release(%d)\n", bus);
}

void spi_transfer_bytes(spi_t bus, spi_cs_t cs, bool cont,
                        const void *out, void *in, size_t len)
{
    const uint8_t *outbuf = out;
    uint8_t *inbuf = in;
    uint8_t  tmp;

    /* make sure at least one input or one output buffer is given */
    assert(outbuf || inbuf);

    /* activate the given chip select line */
    if (cs != SPI_CS_UNDEF) {
        gpio_clear((gpio_t)cs);
#ifdef SPI_CS_UNDEF_MAPPED_ON_CLK_EN
    }else if (spi_config[bus].cs_pin == GPIO_UNDEF) {
        WAIT_SPI_IDLE(dev(bus));
        dev(bus)->clk_en = 1;
#endif
    }

    /* transfer data, use shortpath if only sending data */
    /* NOTE: we are bidirectional, for each byte sent, we must read a byte.
     * 		i.e. cc1101 sends us the status byte while we are sending.
     * WARNING: traces here might lead to radio fifo underflow...
     * -jm-
     */
#if 1
#define SPI_DEBUG DEBUG
#else
#define SPI_DEBUG (void*)
#endif
    if (!inbuf) {
    	SPI_DEBUG(" aps_spi.o");
        for (size_t i = 0; i < len; i++) {
            /* make sure the transfer is completed before continuing */
            WAIT_SPI_IDLE(dev(bus));
            while (!(dev(bus)->tx_status & APS_SPI_TX_SR_FIFO_NOT_FULL)) {} // wait until tx fifo is available
            dev(bus)->tx_data = outbuf[i];
            WAIT_SPI_IDLE_OPTIONAL(dev(bus));
            while (!(dev(bus)->rx_status & APS_SPI_RX_SR_DATA_AVAILABLE)) {} // wait until rx fifo is available
            tmp = (uint8_t)dev(bus)->rx_data; if (tmp) {}; /* force a discarded ld [sfr] */
            SPI_DEBUG("<%02x,%02x>",tmp,outbuf[i]);
        }
    }
    else if (!outbuf) {
    	SPI_DEBUG(" aps_spi.i");
        for (size_t i = 0; i < len; i++) {
            /* make sure the transfer is completed before continuing */
            WAIT_SPI_IDLE(dev(bus));
            while (!(dev(bus)->tx_status & APS_SPI_TX_SR_FIFO_NOT_FULL)) {} // wait until tx fifo is available
            dev(bus)->tx_data = 0;
            WAIT_SPI_IDLE_OPTIONAL(dev(bus));
            while (!(dev(bus)->rx_status & APS_SPI_RX_SR_DATA_AVAILABLE)) {} // wait until rx fifo is available
            inbuf[i] = (uint8_t)dev(bus)->rx_data;
            SPI_DEBUG("<%02x,  >",inbuf[i]);
        }
    }
    else {
    	SPI_DEBUG(" aps_spi.b");
        for (size_t i = 0; i < len; i++) {
            /* make sure the transfer is completed before continuing */
            WAIT_SPI_IDLE(dev(bus));
            while (!(dev(bus)->tx_status & APS_SPI_TX_SR_FIFO_NOT_FULL)) {} // wait until tx fifo is available
            dev(bus)->tx_data = outbuf[i];
            WAIT_SPI_IDLE_OPTIONAL(dev(bus));
            while (!(dev(bus)->rx_status & APS_SPI_RX_SR_DATA_AVAILABLE)) {} // wait until rx fifo is available
            inbuf[i] = (uint8_t)dev(bus)->rx_data;
            SPI_DEBUG("<%02x,%02x>",inbuf[i],outbuf[i]);
        }
    }
    SPI_DEBUG("#%d\n",(int)len);

    /* make sure the transfer is completed before continuing */
    WAIT_SPI_IDLE(dev(bus));

    /* release the chip select if not specified differently */
    if (!cont)
    {
        if (cs != SPI_CS_UNDEF) {
            gpio_set((gpio_t)cs);
#ifdef SPI_CS_UNDEF_MAPPED_ON_CLK_EN
        }else if (spi_config[bus].cs_pin == GPIO_UNDEF) {
            WAIT_SPI_IDLE(dev(bus));
            dev(bus)->clk_en = 0;
#endif
        }
    }
}

#endif /*USE_PERIPH_CORTUS_SPI*/
