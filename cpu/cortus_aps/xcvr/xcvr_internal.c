/*
 * Copyright (c) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_ciot25_xcvr
 * @{
 * @file
 * @brief       implementation of internal functions for ciot25 xcvr
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "hwatomic.h"
#include "hwsystem.h"


#include "periph_cpu.h"

#include "xcvr.h"
#include "xcvr_internal.h"
#include "xcvr_registers.h"

#define ENABLE_DEBUG 1

#if ENABLE_DEBUG
#include "debug.h"
#else
    #define DEBUG(...)
#endif

int xcvr_check_version(const ciot25_xcvr_t *dev)
{
    (void) dev;
    DEBUG("[xcvr] check version");
    // TODO
    return 0;
}

void xcvr_write_fifo(const ciot25_xcvr_t *dev, uint8_t *buffer, uint8_t size)
{
    (void) dev;
    uint8_t cnt_byte = 0 ;
    for (cnt_byte = 0; cnt_byte < size; cnt_byte++){
        dif->tx_data = buffer[cnt_byte];
    }
}

void xcvr_read_fifo(const ciot25_xcvr_t *dev, uint8_t *buffer, uint8_t size)
{
    (void) dev;
    uint8_t cnt_byte = 0 ;
    for (cnt_byte = 0; cnt_byte < size; cnt_byte++){
        buffer[cnt_byte] = dif->rx_data ;
    }
}

void xcvr_rx_chain_calibration(ciot25_xcvr_t *dev)
{
    (void) dev;
    DEBUG("[xcvr] RX calibration");
    //TODO
}

int16_t xcvr_read_rssi(ciot25_xcvr_t *dev)
{
    int16_t rssi = 0;
    uint32_t old_rssi_cfg = 0;

    if (dev->settings.state != XCVR_RF_RX_RUNNING)
    {
        xcvr_set_state(dev, XCVR_RF_RX_RUNNING);
        /* Set radio in continuous reception */
        xcvr_set_op_mode(dev, XCVR_OPMODE_RECEIVER);
        old_rssi_cfg = baseband->rssi_config;
        baseband->rssi_config = (baseband->rssi_config & ~XCVR_BASEBAND_RSSI_CONFIG_MEASUREMENT_ONLY) | XCVR_BASEBAND_RSSI_CONFIG_MEASUREMENT_ONLY;
        baseband->rssi_config = (baseband->rssi_config & ~XCVR_BASEBAND_RSSI_CONFIG_MEASURE_PROCESSING_ON) | XCVR_BASEBAND_RSSI_CONFIG_MEASURE_PROCESSING_ON;
        hw_busy_wait(700); // wait settling time and RSSI smoothing time
    }
    else {
        old_rssi_cfg = baseband->rssi_config;
        baseband->rssi_config = (baseband->rssi_config & ~XCVR_BASEBAND_RSSI_CONFIG_MEASUREMENT_ONLY) | XCVR_BASEBAND_RSSI_CONFIG_MEASUREMENT_ONLY;
        baseband->rssi_config = (baseband->rssi_config & ~XCVR_BASEBAND_RSSI_CONFIG_MEASURE_PROCESSING_ON) | XCVR_BASEBAND_RSSI_CONFIG_MEASURE_PROCESSING_ON;
        hw_busy_wait(700); // wait settling time and RSSI smoothing time
    }

    rssi = ((int8_t)baseband->rssi_inst_val) - 30;
    baseband->rssi_config = old_rssi_cfg;
    return rssi;
}


bool xcvr_is_channel_free(ciot25_xcvr_t *dev, uint32_t freq, int16_t rssi_threshold)
{
    int16_t rssi = 0;

    xcvr_set_channel(dev, freq, true);
    xcvr_set_op_mode(dev, XCVR_OPMODE_RECEIVER);

    hw_busy_wait(1000); /* wait 1 millisecond */

    rssi = xcvr_read_rssi(dev);
    xcvr_set_standby(dev);

    return (rssi <= rssi_threshold);
}

void xcvr_flush_rx_fifo(void)
{
    //if (dif->rx_status & XCVR_DATA_IF_RX_NOT_EMPTY)
    {
        dif->rx_flush = 1;
        asm volatile ("mov r0,r0\n");
        while(!(dif->rx_status & XCVR_DATA_IF_RX_FLUSHED));
    }
}

bool xcvr_is_rx_fifo_empty(void)
{
    return (!(dif->rx_status & XCVR_DATA_IF_RX_NOT_EMPTY));
}

void xcvr_flush_tx_fifo(void)
{
    /*if (dif->tx_status & XCVR_DATA_IF_TX_NOT_EMPTY)
    {
        dif->tx_flush = 1;
        dif->tx_flush = 0;
        while(!(dif->tx_status & XCVR_DATA_IF_TX_FLUSHED));
    }*/

    dif->tx_flush = 1;
    asm volatile ("mov r0,r0\n");
    while(!(dif->tx_status & XCVR_DATA_IF_TX_FLUSHED));
}

bool xcvr_is_tx_fifo_empty(void)
{
    return (!(dif->tx_status & XCVR_DATA_IF_TX_NOT_EMPTY));
}
