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
 * @ingroup     drivers_sx127x
 * @{
 * @file
 * @brief       Semtech SX127X internal functions
 *
 * @author      Eugene P. <ep@unwds.com>
 * @author      José Ignacio Alamos <jose.alamos@inria.cl>
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 */

#ifndef SX127X_INTERNAL_H
#define SX127X_INTERNAL_H

#include <inttypes.h>
#include "sx127x.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name   Constant values used to compute RSSI
 * @{
 */
#if defined(MODULE_SX1272)
#define SX127X_RSSI_OFFSET                                 (-139)
#else /* MODULE_SX1276 */
#define SX127X_RSSI_OFFSET_LF                              (-164)
#define SX127X_RSSI_OFFSET_HF                              (-157)
#endif
/** @} */

/**
 * @name   Device specific logic level to indicate POR-cycle is active
 * @{
 */
#if defined(MODULE_SX1272)
#define SX127X_POR_ACTIVE_LOGIC_LEVEL                      (1)
#else /* MODULE_SX1276 */
#define SX127X_POR_ACTIVE_LOGIC_LEVEL                      (0)
#endif
/** @} */

/**
 * @brief   Check the transceiver version
 *
 * @param[in] dev                      The sx127x device descriptor
 *
 * @return 0 when a valid device version is found
 * @return -1 when no valid device version is found
 */
int sx127x_check_version(const sx127x_t *dev);

/**
 * @brief   Writes the radio register at specified address.
 *
 * @param[in] dev                      The sx127x device structure pointer
 * @param[in] addr                     Register address
 * @param[in] data                     New register value
 */
void sx127x_reg_write(const sx127x_t *dev, uint8_t addr, uint8_t data);

/**
 * @brief   Writes the 2 bytes radio register at specified MSB address.
 *
 * @param[in] dev                      The sx127x device structure pointer
 * @param[in] msbaddr                  MSB Register address
 * @param[in] data                     New register value
 */
void sx127x_reg_write_u16(const sx127x_t *dev, uint8_t msbaddr, uint16_t data);

/**
 * @brief   Reads the radio register at specified address.
 *
 * @param[in] dev                      The sx127x device structure pointer
 * @param[in] addr                     Register address
 *
 * @return    Register value
 */
uint8_t sx127x_reg_read(const sx127x_t *dev, uint8_t addr);

/**
 * @brief   Reads the 2 bytes radio register at specified MSB address.
 *
 * @param[in] dev                      The sx127x device structure pointer
 * @param[in] addr                     Register address
 *
 * @return    Register value
 */
uint16_t sx127x_reg_read_u16(const sx127x_t *dev, uint8_t msbaddr);

/**
 * @brief   Writes multiple radio registers starting at address (burst-mode).
 *
 * @param[in] dev                      The sx127x device structure pointer
 * @param[in] addr                     First radio register address
 * @param[in] buffer                   Buffer containing the new register's values
 * @param[in] size                     Number of registers to be written
 */
void sx127x_reg_write_burst(const sx127x_t *dev, uint8_t addr, uint8_t *buffer,
                            uint8_t size);

/**
 * @brief   Reads multiple radio registers starting at address.
 *
 * @param[in]  dev                     The sx127x device structure pointer
 * @param[in]  addr                    First radio register address
 * @param[in]  size                    Number of registers to be read
 * @param[out] buffer                  Buffer where to copy registers data
 */
void sx127x_reg_read_burst(const sx127x_t *dev, uint8_t addr, uint8_t *buffer,
                           uint8_t size);

/**
 * @brief   Writes the buffer contents to the SX1276 FIFO
 *
 * @param[in] dev                      The sx127x device structure pointer
 * @param[in] buffer                   Buffer Buffer containing data to be put on the FIFO.
 * @param[in] size                     Size Number of bytes to be written to the FIFO
 */
void sx127x_write_fifo(const sx127x_t *dev, uint8_t *buffer, uint8_t size);

/**
 * @brief   Reads the contents of the SX1276 FIFO
 *
 * @param[in] dev                      The sx127x device structure pointer
 * @param[in] size                     Size Number of bytes to be read from the FIFO
 * @param[out] buffer                  Buffer Buffer where to copy the FIFO read data.
 */
void sx127x_read_fifo(const sx127x_t *dev, uint8_t *buffer, uint8_t size);

#ifdef _APS_OSS7_
/**
 * @brief   Flush the FIFO.
 *
 * @param[in] dev                      The sx127x device descriptor
 */
void sx127x_flush_fifo(const sx127x_t *dev);

/**
 * @brief   check if the FIFO is empty.
 *
 * @param[in] dev                      The sx127x device descriptor
 *
 * @return TRUE/FALSE to indicate whether the FIFO is empty or not
 */
bool sx127x_is_fifo_empty(const sx127x_t *dev);
#endif

#if defined(MODULE_SX1276)
/**
 * @brief   Performs the Rx chain calibration for LF and HF bands
 *
 *          Must be called just after the reset so all registers are at their
 *          default values
 *
 * @param[in] dev                      The sx127x device structure pointer
 */
void sx1276_rx_chain_calibration(sx127x_t *dev);
#endif

/* FIXME: declared here due to missing gpio_edge(rising|falling) interface. */
/*        we probably need 2 isr variants instead of extending the gpio api
 *        to avoid gpio reconfiguration and missed irq due to latency... */
/* SX127X DIO interrupt handlers initialization */
void sx127x_on_dio0_isr(void *arg);
void sx127x_on_dio1_isr(void *arg);
void sx127x_on_dio2_isr(void *arg);
void sx127x_on_dio3_isr(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* SX127X_INTERNAL_H */
/** @} */
