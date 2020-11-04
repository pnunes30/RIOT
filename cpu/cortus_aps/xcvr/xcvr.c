/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_xcvr_ciot25
 * @{
 * @file
 * @brief       Basic functionality of xcvr driver
 *
 * @author      Yousra Amrani <yousra.amrani@cortus.com>
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "periph_cpu.h"

#include "xcvr.h"
#include "xcvr_internal.h"
#include "xcvr_registers.h"
#include "xcvr_netdev.h"

#include "d7a/timer.h"
#include "errors.h"

#include "hwsystem.h"

#define ENABLE_DEBUG 0

#if ENABLE_DEBUG
#include "debug.h"
static void log_print_data(uint8_t* message, uint32_t length)
{
    for( uint32_t i=0 ; i<length ; i++ )
    {
        printf(" %02X", message[i]);
    }
    #define DEBUG_DATA(...) log_print_data(__VA_ARGS__)
#else
    #define DEBUG_DATA(...)
#endif

/* Internal functions */
static void _init_timers(ciot25_xcvr_t *dev);
static void _on_tx_timeout(void *arg);
static void _on_rx_timeout(void *arg);

void xcvr_setup(ciot25_xcvr_t *dev)
{
    netdev_t *netdev = (netdev_t*) dev;
    netdev->driver = &xcvr_driver;
}

void xcvr_reset(const ciot25_xcvr_t *dev)
{
    (void)dev;
    //TODO
}

int xcvr_init(ciot25_xcvr_t *dev)
{
    /* reset options */
    dev->options = 0;

    /* set default options */
    xcvr_set_option(dev, XCVR_OPT_TELL_RX_END, true);
    xcvr_set_option(dev, XCVR_OPT_TELL_TX_END, true);

    _init_timers(dev);
    //FIXME Do we need to wait a little bit after the boot?

    xcvr_set_op_mode(dev, XCVR_OPMODE_STANDBY);

    xcvr_rx_chain_calibration(dev);

    return XCVR_INIT_OK;
}

void xcvr_init_radio_settings(ciot25_xcvr_t *dev)
{
    xcvr_set_op_mode(dev, XCVR_OPMODE_STANDBY);

    radio->tx_cfg = XCVR_RADIO_TX_CONFIG_TRANSMISSION_DISABLE |
                    XCVR_RADIO_TX_CONFIG_TRANSMISSION_POWER_CTL_15 |
                    XCVR_RADIO_TX_CONFIG_TRANSMISSION_I_CTL_7 |
                    XCVR_RADIO_TX_CONFIG_DRIVE_ENABLE |
                    XCVR_RADIO_TX_CONFIG_VCO_DIVIDER_0;

    radio->rx_cfg = XCVR_RADIO_RX_CONFIG_LNA_GAIN_ENABLED |
                    //XCVR_RADIO_RX_CONFIG_LNA_ATTENUATOR_ENABLED |  // LNA enabled +20dB + attenuator enabled -20dB ????
                    XCVR_RADIO_RX_CONFIG_LNA_PFB_ENABLE |
                    XCVR_RADIO_RX_CONFIG_LNA_TURBO_MODE_ENABLE | //FIXME 2 bits for enable/disable?
                    XCVR_RADIO_RX_CONFIG_MIXER_ENABLED |
                    XCVR_RADIO_RX_CONFIG_MIXER_GAIN_40_DB |
                    XCVR_RADIO_RX_CONFIG_LNA_AND_MIXER_SET_BY_SFRS |
                    XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_2 |
                    XCVR_RADIO_RX_CONFIG_DC_COMPENSATION_DISABLE | // Not available for V2
                    XCVR_RADIO_RX_CONFIG_DC_COMPENSATION_VALUE_I_32 |
                    XCVR_RADIO_RX_CONFIG_DC_COMPENSATION_VALUE_Q_32;

    radio->pll_cfg = XCVR_RADIO_PLL_CONFIG_PLL_ENABLE |
                     XCVR_RADIO_PLL_CONFIG_PLL_CP_OTA_ENABLE |
                     XCVR_RADIO_PLL_CONFIG_PLL_LOCK_ENABLE |
                     XCVR_RADIO_PLL_CONFIG_VCO_CALIBRATION_DISABLE |
                     XCVR_RADIO_PLL_CONFIG_VCO_REF_VALUE_0 |
                     XCVR_RADIO_PLL_CONFIG_VCO_BAND_SELECT_VALUE_7;

    radio->test_cfg = XCVR_RADIO_TEST_CONFIG_CONTROL |
                      XCVR_RADIO_TEST_CONFIG_CONTROL_33;

    // ADC CFG (the xcvr use rx_adc_clk so we need activate some of the rx part to do a transmit)
    radio->adc_cfg = XCVR_RADIO_ADC_CONFIG_REFERENCE_I_CHANNEL_ENABLE |
                     XCVR_RADIO_ADC_CONFIG_REFERENCE_Q_CHANNEL_ENABLE |
                     XCVR_RADIO_ADC_CONFIG_I_CHANNEL_ENABLE |
                     XCVR_RADIO_ADC_CONFIG_Q_CHANNEL_ENABLE |
                     XCVR_RADIO_ADC_CONFIG_TEST_MODE_1 |
                     XCVR_RADIO_ADC_CONFIG_CALIBRATION_DISABLE |
                     XCVR_RADIO_ADC_CONFIG_TRIMMING_VALUE_30;

    baseband->rx_config = XCVR_BASEBAND_RX_CONFIG_CIC_FILTER_DEC_FAC_32MHZ |
                          XCVR_BASEBAND_RX_CONFIG_CIC_GAIN_0_DB |
                          XCVR_BASEBAND_RX_CONFIG_DEFAULT_HARD_CODED_COEFF |
                          XCVR_BASEBAND_RX_CONFIG_UPPER_SIDEBAND |
                          XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_1000_500;

    baseband->agc_config = XCVR_BASEBAND_AGC_CONFIG_ANALOG_STATE_MACHINE_OFF |
                           XCVR_BASEBAND_AGC_CONFIG_AUTO_ACTIV_GAIN_STATE |
                           XCVR_BASEBAND_AGC_CONFIG_MANUAL_ACTIV_IF_STAGE_OFF |
                           XCVR_BASEBAND_AGC_CONFIG_MANUAL_ACTIV_FE_STAGE_OFF |
                           XCVR_BASEBAND_AGC_CONFIG_RSSI_LOW_DEFAULT_THRESHOLD |
                           XCVR_BASEBAND_AGC_CONFIG_RSSI_HIGH_DEFAULT_THRESHOLD;

    baseband->afc_config = XCVR_BASEBAND_AFC_CONFIG_OFF |
                           XCVR_BASEBAND_AFC_CONFIG_TIMER_OFF |
                           XCVR_BASEBAND_AFC_CONFIG_BOOST_MODE_OFF |
                           XCVR_BASEBAND_AFC_CONFIG_TIME_CONST_DC_REMOVING;

    baseband->dcoc_config = XCVR_BASEBAND_DCOC_CONFIG_ON |
                            XCVR_BASEBAND_DCOC_CONFIG_MANUAL_CORRECTION_OFF |
                            XCVR_BASEBAND_DCOC_CONFIG_TIME_CONSTANT;

    baseband->rssi_config = XCVR_BASEBAND_RSSI_CONFIG_MEASURE_PROCESSING_OFF | // even when not in RX mode????
                            XCVR_BASEBAND_RSSI_CONFIG_MEASURE_AND_CONDI_START |
                            XCVR_BASEBAND_RSSI_CONFIG_DEFAULT_NUM_AVERAGES | //XCVR_BASEBAND_RSSI_CONFIG_NUM_AVERAGES_ZERO |
                            XCVR_BASEBAND_RSSI_CONFIG_FREEZE_V_END_OF_SYNC |
                            XCVR_BASEBAND_RSSI_CONFIG_TIMEOUT_OFF |
                            XCVR_BASEBAND_RSSI_CONFIG_DEFAULT_NUM_SYMBOLS_FREEZE |
                            XCVR_BASEBAND_RSSI_CONFIG_TIMEOUT_TIMER_DEFAULT |
                            XCVR_BASEBAND_RSSI_CONFIG_TM_DEACTIV_IF_M_RSSI_SUP_HI_TH;

    baseband->rssi_threshold = XCVR_BASEBAND_RSSI_DEFAULT_THRESHOLD_LOW |
                              (XCVR_BASEBAND_RSSI_DEFAULT_THRESHOLD_HIGH << 8);

    baseband->dm_config = XCVR_BASEBAND_DM_CONFIG_OFF |
                          XCVR_BASEBAND_DM_CONFIG_AUTO_MAGNITUDE_CONTROL_MODE |
			  XCVR_BASEBAND_DM_CONFIG_DEMULATOR_OUT_55V5 |
                          XCVR_BASEBAND_DM_CONFIG_MODULATION_SHAPING_20;

    baseband->cdr_config1 = XCVR_BASEBAND_CDR_CONFIG2_GL_FILT_ON;

//    baseband->cdr_config2 = XCVR_BASEBAND_CDR_CONFIG2_KP_VALUE |
//                            XCVR_BASEBAND_CDR_CONFIG2_KI_VALUE;

    codec->packet_config = XCVR_CODEC_PACKET_CONFIG_SYNC_TWO_BYTES |
                           XCVR_CODEC_PACKET_CONFIG_PREAMBLE_LENGTH_128_BITS |
                           XCVR_CODEC_PACKET_CONFIG_PREAMBLE_POLARITY_0XAA |
                           XCVR_CODEC_PACKET_CONFIG_PN9_OFF |
                           XCVR_CODEC_PACKET_CONFIG_FEC_OFF |
                           XCVR_CODEC_PACKET_CONFIG_CRC_ON |
                           XCVR_CODEC_PACKET_CONFIG_LEN_VAR |
                           XCVR_CODEC_PACKET_CONFIG_SYNC_MODE_ENABLE;//DISABLE;

    codec->detector_config = XCVR_CODEC_DETECTOR_CONFIG_SELECTION |
                             XCVR_CODEC_DETECTOR_CONFIG_PREAMBLE_THRESHOLD_16 |
                             XCVR_CODEC_DETECTOR_CONFIG_SYNC_THRESHOLD_13 |
                             XCVR_CODEC_DETECTOR_CONFIG_PREAMBLE_TIMEOUT_OFF |
                             XCVR_CODEC_DETECTOR_CONFIG_SYNC_TIMEOUT_OFF |
                             XCVR_CODEC_DETECTOR_CONFIG_PREAMBLE_TIMEOUT_VALUE |
                             XCVR_CODEC_DETECTOR_CONFIG_SYNC_TIMEOUT_VALUE;

    codec->corr_detector_config = XCVR_CODEC_CORR_DETECTOR_CONFIG_CORRELATION_OFF |
                                  XCVR_CODEC_CORR_DETECTOR_CONFIG_PREAMBLE_THRESHOLD |
                                  XCVR_CODEC_CORR_DETECTOR_CONFIG_SYNC_THRESHOLD;

    dif->tx_thr = XCVR_DATA_IF_TX_THRESHOLD;
    dif->rx_thr = XCVR_DATA_IF_RX_THRESHOLD;

    // XCVR CONFIG
    xcvr->freq_ref = XCVR_FREQ_REF;
    xcvr->prescaler = XCVR_FSK_PRESCALER;
    xcvr->clkgen_en = XCVR_CLKGEN_EN;

    xcvr_set_tx_timeout(dev, XCVR_TX_TIMEOUT_DEFAULT);

    xcvr_set_channel(dev, XCVR_CHANNEL_DEFAULT, true);

    xcvr_set_payload_length(dev, XCVR_PAYLOAD_LENGTH);
    xcvr_set_tx_power(dev, XCVR_RADIO_TX_POWER);
}

static void _on_tx_timeout(void *arg)
{
    netdev_t *dev = (netdev_t *) arg;

    dev->event_callback(dev, NETDEV_EVENT_TX_TIMEOUT);
}

static void _on_rx_timeout(void *arg)
{
    netdev_t *dev = (netdev_t *) arg;

    dev->event_callback(dev, NETDEV_EVENT_RX_TIMEOUT);
}

static void _init_timers(ciot25_xcvr_t *dev)
{
    dev->_internal.tx_timeout_timer.arg = dev;
    dev->_internal.tx_timeout_timer.cb = _on_tx_timeout;

    dev->_internal.rx_timeout_timer.arg = dev;
    dev->_internal.rx_timeout_timer.cb = _on_rx_timeout;
}
