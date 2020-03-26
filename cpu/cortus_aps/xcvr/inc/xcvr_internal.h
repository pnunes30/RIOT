/*
 * Copyright (C) 2016 Unwired Devices <info@unwds.com>
 *               2017 Inria Chile
 *               2017 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_xcvr
 * @{
 * @file
 * @brief       cortus XCVR internal functions
 *
 * @author
 */

#ifndef XCVR_INTERNAL_H
#define XCVR_INTERNAL_H

#include <inttypes.h>

#include "xcvr.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief   Check the transceiver version
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return 0 when a valid device version is found
 * @return -1 when no valid device version is found
 */
int xcvr_check_version(const ciot25_xcvr_t *dev);

/**
 * @brief   Writes the buffer contents to the xcvr FIFO
 *
 * @param[in] dev                      The xcvr device structure pointer
 * @param[in] buffer                   Buffer Buffer containing data to be put on the FIFO.
 * @param[in] size                     Size Number of bytes to be written to the FIFO
 */
void xcvr_write_fifo(const ciot25_xcvr_t *dev, uint8_t *buffer, uint8_t size);

/**
 * @brief   Reads the contents of the xcvr FIFO
 *
 * @param[in] dev                      The xcvr device structure pointer
 * @param[in] size                     Size Number of bytes to be read from the FIFO
 * @param[out] buffer                  Buffer Buffer where to copy the FIFO read data.
 */
void xcvr_read_fifo(const ciot25_xcvr_t *dev, uint8_t *buffer, uint8_t size);

/**
 * @brief   Performs the Rx chain calibration for LF and HF bands
 *
 *          Must be called just after the reset so all registers are at their
 *          default values
 *
 * @param[in] dev                      The xcvr device structure pointer
 */
void xcvr_rx_chain_calibration(ciot25_xcvr_t *dev);


/**
 * @brief   Flush the TX FIFO.
 *
 */
void xcvr_flush_tx_fifo(void);

/**
 * @brief   check if the TX FIFO is empty.
 *
 * @return TRUE/FALSE to indicate whether the TX FIFO is empty or not
 */
bool xcvrx_is_tx_fifo_empty(void);

/**
 * @brief   Flush the RX FIFO.
 */
void xcvr_flush_rx_fifo(void);

/**
 * @brief   check if the RX FIFO is empty.
 *
 * @return TRUE/FALSE to indicate whether the RX FIFO is empty or not
 */
bool xcvr_is_rx_fifo_empty(void);


#ifdef __cplusplus
}
#endif

#endif /* XCVR_INTERNAL_H */
/** @} */
