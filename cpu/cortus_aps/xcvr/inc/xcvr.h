/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    driver_xcvr CIoT25 radio driver
 * @ingroup     driver_xcvr
 * @brief       Driver for the CIoT25 transceiver.
 *
 * This module contains the driver for the transceiver of the CIoT25 chip
 *
 * @{
 *
 * @file
 * @brief       Public interface for CIoT25 XCVR driver
 * @author      philippe.nunes@cortus.com
 */

#ifndef XCVR_H
#define XCVR_H

#include "d7a/timer.h"
#include "net/netdev.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    XCVR device default configuration
 * @{
 */
#define XCVR_PACKET_LENGTH             (0xFF)             /**< max packet length = 255b */
#define XCVR_FIFO_MAX_SIZE             (64)               /**< FIFO max size */
#define XCVR_FIFO_TX_MAX_SIZE          (11)               /**< FIFO max size */
#define XCVR_FIFO_MID_SIZE             (32)               /**< FIFO mid size */

#define XCVR_MODEM_FSK                 (0)                /**< Use FSK as default modem */
#define XCVR_CHANNEL_DEFAULT           (868300000UL)      /**< Default channel frequency, 868.3MHz (Europe) */

/* FIXME TO CONFIRM */
#define XCVR_HF_CHANNEL_DEFAULT        (868000000UL)      /**< Use to calibrate RX chain for LF and HF bands */
#define XCVR_RF_MID_BAND_THRESH        (525000000UL)      /**< Mid-band threshold used to set the channel */

#define XCVR_XTAL_FREQ                 (32000000UL)       /**< Internal oscillator frequency, 32MHz */
#define XCVR_WAKEUP_TIME               (1000U)            /**< In microseconds [us] */

/* BW NOT SET WITH CIOT25 XCVR */
//#define XCVR_BW_125_KHZ              125
//#define XCVR_BW_DEFAULT              (SX127X_BW_125_KHZ) /**< Set default bandwidth to 125kHz */

#define XCVR_PAYLOAD_LENGTH            (0U)                /**< Set payload length, unused with implicit header */

#define XCVR_TX_TIMEOUT_DEFAULT        (1000U * 1000U * 30UL) /**< TX timeout, 30s */
#define XCVR_RX_SINGLE                 (false)                /**< Single byte receive mode => continuous by default */
#define XCVR_RADIO_TX_POWER            (14U)                  /**< Radio power in dBm */

/** @} */

/**
 * @name    Internal device option flags
 * @{
 */
#define XCVR_OPT_TELL_TX_START    (0x01)    /**< notify MAC layer on TX start */
#define XCVR_OPT_TELL_TX_END      (0x02)    /**< notify MAC layer on TX finished */
#define XCVR_OPT_TELL_RX_START    (0x04)    /**< notify MAC layer on RX start */
#define XCVR_OPT_TELL_RX_END      (0x08)    /**< notify MAC layer on RX finished */
#define XCVR_OPT_TELL_TX_REFILL   (0x10)    /**< notify MAC layer when TX needs to be refilled */
#define XCVR_OPT_PRELOADING       (0x20)    /**< preloading enabled */
/** @} */

/**
 * @brief   XCVR initialization result.
 */
enum {
    XCVR_INIT_OK = 0,                /**< Initialization was successful */
    XCVR_ERR_NODEV                   /**< No valid device version found */
};

/**
 * @brief   Radio driver supported modems.
 */

/**
 * @brief   Radio driver internal state machine states definition.
 */
enum {
    XCVR_RF_IDLE = 0,                /**< Idle state */
    XCVR_RF_RX_RUNNING,              /**< Sending state */
    XCVR_RF_TX_RUNNING,              /**< Receiving state */
    XCVR_RF_CAD,                     /**< Channel activity detection state */
};

/**
 * @brief   Event types.
 */
enum {
    XCVR_RX_DONE = 0,                /**< Receiving complete */
    XCVR_TX_DONE,                    /**< Sending complete*/
    XCVR_RX_TIMEOUT,                 /**< Receiving timeout */
    XCVR_TX_TIMEOUT,                 /**< Sending timeout */
    XCVR_RX_ERROR_CRC,               /**< Receiving CRC error */
    XCVR_FHSS_CHANGE_CHANNEL,        /**< Channel change */
    XCVR_CAD_DONE,                   /**< Channel activity detection complete */
};

/**
 * @name    XCVR device descriptor boolean flags
 * @{
 */
#define XCVR_LOW_DATARATE_OPTIMIZE_FLAG       (1 << 0)
#define XCVR_ENABLE_FIXED_HEADER_LENGTH_FLAG  (1 << 1)
#define XCVR_ENABLE_CRC_FLAG                  (1 << 2)
#define XCVR_CHANNEL_HOPPING_FLAG             (1 << 3)
#define XCVR_IQ_INVERTED_FLAG                 (1 << 4)
#define XCVR_RX_CONTINUOUS_FLAG               (1 << 5)
/** @} */


/**
 * @brief   Fsk configuration structure.
 */
typedef struct {
    uint16_t preamble_len;             /**< Length of preamble header */
    uint8_t sync_len;                  /**< Length of sync word */
    uint8_t power;                     /**< Signal power */
    uint8_t bandwidth;                 /**< Signal bandwidth */
    uint8_t datarate;                  /**< bitrate in bps */

    uint8_t flags;                     /**< Boolean flags */
    uint32_t rx_timeout;               /**< RX timeout in symbols */
    uint32_t tx_timeout;               /**< TX timeout in symbols */
} xcvr_fsk_settings_t;

/**
 * @brief   Radio settings.
 */
typedef struct {
    uint32_t channel;                  /**< Radio channel */
    uint8_t state;                     /**< Radio state */
    xcvr_fsk_settings_t fsk;           /**< Fsk settings */
} xcvr_radio_settings_t;

/**
 * @brief   xcvr internal data.
 */
typedef struct {
    /* Data that will be passed to events handler in application */
    timer_event tx_timeout_timer;         /**< TX operation timeout timer */
    timer_event rx_timeout_timer;         /**< RX operation timeout timer */
} xcvr_internal_t;

/**
 * @brief   xcvr IRQ flags.
 */
typedef uint8_t xcvr_flags_t;

/**
 * @brief struct holding xcvr packet + metadata
 */
typedef struct {
    uint16_t length;                     /**< Length of the packet (without length byte) */
    uint8_t pos;                         /**< Index of the data already transmitted. */
    uint8_t fifothresh;                  /**< Threshold used to trigger FifoLevel interrupt. */
    uint8_t buf[XCVR_PACKET_LENGTH +1];/**< buffer for the whole packet including the length byte */
    uint8_t crc_status;                  /**< CRC status of the received packet */
} xcvr_pkt_t;

/**
 * @brief   CIOT25 XCVR device descriptor.
 * @extends netdev_t
 */
typedef struct {
    netdev_t netdev;                 /**< Netdev parent struct */
    xcvr_radio_settings_t settings;  /**< Radio settings */
    xcvr_internal_t _internal;       /**< Internal xcvr data used within the driver */
    xcvr_flags_t irq_flags;          /**< Device IRQ flags */
    uint16_t options;                /**< Option flags */
    xcvr_pkt_t packet;               /**< RX/TX buffer */
} ciot25_xcvr_t;

typedef struct
{
    char type[32];
    char field[32];
    uint32_t value;
} sfr_register_t;

/**
 * @brief   Setup the xcvr
 *
 * @param[in] dev                      Device descriptor
 * @param[in] params                   Parameters for device initialization
 */
void xcvr_setup(ciot25_xcvr_t *dev);

/**
 * @brief   Resets the xcvr
 *
 * @param[in] dev                      The xcvr device descriptor
 */
void xcvr_reset(const ciot25_xcvr_t *dev);

/**
 * @brief   Initializes the transceiver.
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return result of initialization
 */
int xcvr_init(ciot25_xcvr_t *dev);

/**
 * @brief   Initialize radio settings with default values
 *
 * @param[in] dev                      The xcvr device descriptor
 */
void xcvr_init_radio_settings(ciot25_xcvr_t *dev);

/**
 * @brief   Checks that channel is free with specified RSSI threshold.
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] freq                     channel RF frequency
 * @param[in] rssi_threshold           RSSI threshold
 *
 * @return true if channel is free, false otherwise
 */
bool xcvr_is_channel_free(ciot25_xcvr_t *dev, uint32_t freq, int16_t rssi_threshold);

/**
 * @brief   Reads the current RSSI value.
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return the current value of RSSI (in dBm)
 */
int16_t xcvr_read_rssi(ciot25_xcvr_t *dev);

/**
 * @brief   Gets current state of transceiver.
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return radio state [RF_IDLE, RF_RX_RUNNING, RF_TX_RUNNING]
 */
uint8_t xcvr_get_state(const ciot25_xcvr_t *dev);

/**
 * @brief   Sets current state of transceiver.
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] state                    The new radio state
 *
 * @return radio state [RF_IDLE, RF_RX_RUNNING, RF_TX_RUNNING]
 */
void xcvr_set_state(ciot25_xcvr_t *dev, uint8_t state);

/**
 * @brief   Gets the xcvr syncword length
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return the syncword length
 */
uint8_t xcvr_get_syncword_length(const ciot25_xcvr_t *dev);

/**
 * @brief   Sets the xcvr syncword length
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] len                      The syncword length
 */
void xcvr_set_syncword_length(ciot25_xcvr_t *dev, uint8_t len);

/**
 * @brief   Gets the synchronization word.
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return The syncword length placed in the sync word buffer
 */
uint8_t xcvr_get_syncword(const ciot25_xcvr_t *dev, uint8_t *syncword, uint8_t sync_size);

/**
 * @brief   Sets the synchronization word.
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] syncword                 The synchronization word
 */
void xcvr_set_syncword(ciot25_xcvr_t *dev, uint8_t *syncword, uint8_t sync_size);

/**
 * @brief   Gets the channel RF frequency.
 *
 * @param[in]  dev                     The xcvr device descriptor
 *
 * @return The channel frequency
 */
uint32_t xcvr_get_channel(const ciot25_xcvr_t *dev);

/**
 * @brief   Sets the channel RF frequency.
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] freq                     Channel RF frequency
 */
void xcvr_set_channel(ciot25_xcvr_t *dev, uint32_t freq);

/**
 * @brief   Sets the radio in stand-by mode
 *
 * @param[in] dev                      The xcvr device descriptor
 */
void xcvr_set_standby(ciot25_xcvr_t *dev);

/**
 * @brief   Sets the radio in reception mode.
 *
 * @param[in] dev                      The xcvr device descriptor
 */
void xcvr_set_rx(ciot25_xcvr_t *dev);

/**
 * @brief   Sets the radio in transmission mode.
 *
 * @param[in] dev                      The xcvr device descriptor
 */
void xcvr_set_tx(ciot25_xcvr_t *dev);

/**
 * @brief   Gets the maximum payload length.
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return The maximum payload length
 */
uint8_t xcvr_get_max_payload_len(const ciot25_xcvr_t *dev);

/**
 * @brief   Sets the maximum payload length.
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] maxlen                   Maximum payload length in bytes
 */
void xcvr_set_max_payload_len(ciot25_xcvr_t *dev, uint16_t maxlen);

/**
 * @brief   Gets the xcvr operating mode
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return The actual operating mode
 */
uint8_t xcvr_get_op_mode(const ciot25_xcvr_t *dev);

/**
 * @brief   Sets the xcvr operating mode
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] op_mode                  The new operating mode
 */
void xcvr_set_op_mode(const ciot25_xcvr_t *dev, uint8_t op_mode);

/**
 * @brief   Checks if the xcvr CRC verification mode is enabled
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return the CRC check mode
 */
bool xcvr_get_crc(const ciot25_xcvr_t *dev);

/**
 * @brief   Enable/Disable the xcvr CRC verification mode
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] crc                      The CRC check mode
 */
void xcvr_set_crc(ciot25_xcvr_t *dev, bool crc);

/**
 * @brief   Gets the xcvr payload length
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return the payload length
 */
uint8_t xcvr_get_payload_length(const ciot25_xcvr_t *dev);

/**
 * @brief   Sets the xcvr payload length
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] len                      The payload length
 */
void xcvr_set_payload_length(ciot25_xcvr_t *dev, uint8_t len);

/**
 * @brief   Gets the xcvr TX radio power
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return the radio power
 */
uint8_t xcvr_get_tx_power(const ciot25_xcvr_t *dev);

/**
 * @brief   Sets the xcvr transmission power
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] power                    The TX power
 */
void xcvr_set_tx_power(ciot25_xcvr_t *dev, int8_t power);

/**
 * @brief   Gets the xcvr preamble length
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return the preamble length
 */
uint16_t xcvr_get_preamble_length(const ciot25_xcvr_t *dev);

/**
 * @brief   Sets the xcvr preamble length
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] len                      The preamble length
 */
void xcvr_set_preamble_length(ciot25_xcvr_t *dev, uint8_t len);

/**
 * @brief   Sets the xcvr RX timeout
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] timeout                  The RX timeout
 */
void xcvr_set_rx_timeout(ciot25_xcvr_t *dev, uint32_t timeout);

/**
 * @brief   Sets the xcvr TX timeout
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] timeout                  The TX timeout
 */
void xcvr_set_tx_timeout(ciot25_xcvr_t *dev, uint32_t timeout);

/**
 * @brief   Gets the xcvr bit rate
 *
 * @param[in] dev                      The xcvr device descriptor
 *
 * @return the bit rate (bps)
 */
uint32_t xcvr_get_bitrate(const ciot25_xcvr_t *dev);

/**
 * @brief   Sets the xcvr bit rate
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] bps                      The bit rate
 */
void xcvr_set_bitrate(ciot25_xcvr_t *dev, uint32_t bps);

/**
 * @brief   Enable/Disable the xcvr pll lock mode
 *
 * @param[in] dev                      The xcvr device descriptor
 * @param[in] enable                   The value to put inside register
 */
void xcvr_set_pll(ciot25_xcvr_t *dev, bool enable);

uint8_t xcvr_get_preamble_polarity(const ciot25_xcvr_t *dev);
void xcvr_set_preamble_polarity(ciot25_xcvr_t *dev, bool polarity);

void xcvr_set_rssi_threshold(ciot25_xcvr_t *dev, uint8_t rssi_thr);
uint8_t xcvr_get_rssi_threshold(const ciot25_xcvr_t *dev);

void xcvr_set_rssi_smoothing(ciot25_xcvr_t *dev, uint8_t rssi_samples);
uint8_t xcvr_get_rssi_smoothing(const ciot25_xcvr_t *dev);

void xcvr_set_sync_preamble_detect_on(ciot25_xcvr_t *dev, bool enable);
uint8_t xcvr_get_sync_preamble_detect_on(const ciot25_xcvr_t *dev);

void xcvr_set_dc_free(ciot25_xcvr_t *dev, uint8_t encoding_scheme);
uint8_t xcvr_get_dc_free(const ciot25_xcvr_t *dev);

void xcvr_set_fec_on(ciot25_xcvr_t *dev, bool enable);
uint8_t xcvr_get_fec_on(const ciot25_xcvr_t *dev);

void xcvr_set_tx_fdev(ciot25_xcvr_t *dev, uint32_t fdev);
uint32_t xcvr_get_tx_fdev(const ciot25_xcvr_t *dev);

void xcvr_set_modulation_shaping(ciot25_xcvr_t *dev, uint8_t shaping);
uint8_t xcvr_get_modulation_shaping(const ciot25_xcvr_t *dev);

int xcvr_set_option(ciot25_xcvr_t *dev, uint8_t option, bool state);

void xcvr_set_register(ciot25_xcvr_t *dev, sfr_register_t *sfr_register);
void xcvr_get_register(ciot25_xcvr_t *dev, sfr_register_t *sfr_register);

void xcvr_restart_rx_chain(ciot25_xcvr_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XCVR_H */
/** @} */
