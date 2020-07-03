/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_ciot25_xcvr
 * @{
 * @file
 * @brief       Implementation of get and set functions for CIOT25 XCVR
 *
 * @author
 * @}
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "periph_cpu.h"

//#include "platform.h"
#include "debug.h"
#include "hwradio.h"
#include "hwcounter.h"

#include "xcvr.h"
#include "xcvr_internal.h"
#include "xcvr_registers.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

typedef struct
{
    const char *name; /**< Name of the register*/
} register_t;

typedef struct
{
    const char *name; /**< Name of the register block*/
    uint32_t base_address;
    register_t *register_list;
} register_block_t;

const register_t xcvr_register_list[] = {
    {"status"},
    {"mask"},
    {"op_mode"},
    {"freq_ref"},
    {"bitrate"},
    {"freq_carrier"},
    {"freq_lowif"},
    {"freq_dev"},
    {"ber"},
    {"clkgen_en"},
    {"clk_divider"},
    {"prescaler"},
    NULL
};

const register_t codec_register_list[] = {
    {"packet_config"},
    {"packet_len"},
    {"sync_word_l"},
    {"sync_word_h"},
    {"detector_config"},
    {"corr_detector_config"},
    {"preamble_det_ndet"},
    {"preamble_quality"},
    {"sync_quality"},
    {"payload_quality"},
    NULL
};

const register_t baseband_register_list[] = {
    {"rx_config"},
    {"agc_config"},
    {"dcoc_config"},
    {"dcoc_man_offs"},
    {"dcoc_offs"},
    {"iq_comp_config"},
    {"iq_comp_man_corr"},
    {"iq_comp_corr"},
    {"afc_config"},
    {"afc_offs"},
    {"rssi_config"},
    {"rssi_threshold"},
    {"rssi_inst_val"},
    {"rssi_mean_val"},
    {"rssi_latch_inst_val"},
    {"rssi_latch_mean_val"},
    {"dm_config"},
    {"cdr_config1"},
    {"cdr_config2"},
    {"lp_fir_coef_0_1"},
    {"lp_fir_coef_2_3"},
    {"lp_fir_coef_4_5"},
    {"lp_fir_coef_6_7"},
    {"lp_fir_coef_8_9"},
    {"lp_fir_coef_10_11"},
    {"lp_fir_coef_12_13"},
    {"lp_fir_coef_14_15"},
    {"lp_fir_coef_16_17"},
    {"lp_fir_coef_18_19"},
    {"lp_fir_coef_20_21"},
    {"lp_fir_coef_22_23"},
    {"lp_fir_coef_24_25"},
    {"lp_fir_coef_26_27"},
    {"lp_fir_coef_28_29"},
    {"lp_fir_coef_30_31"},
    {"lp_fir_coef_32_33"},
    {"tx_mod_man"},
};

const register_t radio_register_list[] = {
    {"adc_cfg"},
    {"pll_cfg"},
    {"pll_status"},
    {"rx_cfg"},
    {"tx_cfg"},
    {"test_cfg"},
    NULL
};

const register_t data_if_register_list[] = {
    {"tx_data"},
    {"tx_thr"},
    {"tx_status"},
    {"tx_mask"},
    {"tx_flush"},
    {"rx_data"},
    {"rx_thr"},
    {"rx_status"},
    {"rx_mask"},
    {"rx_flush"},
    NULL
};

const register_block_t register_bloc_list[] = {
    {"xcvr", SFRADR_XCVR, xcvr_register_list},
    {"codec", SFRADR_CODEC, codec_register_list},
    {"baseband", SFRADR_BASEBAND, baseband_register_list},
    {"radio", SFRADR_RADIO, radio_register_list},
    {"dataif", SFRADR_DATA_IF, data_if_register_list}
};

uint32_t _get_addr(unsigned char *type, unsigned char *field)
{
    register_block_t *entry;
    uint8_t nb = sizeof(register_bloc_list) / sizeof(entry);

    DEBUG("size %d", nb);

    /* iterating over command_lists */
    for (unsigned int i = 0; i < 5; i++) {
        if ((entry = &register_bloc_list[i])) {
            /* iterating over commands in command_lists entry */
            if (entry->name != NULL) {
                if (strcmp(entry->name, type) == 0)
                {
                    int j = 0;
                    register_t *register_name = &entry->register_list[j];

                    while (register_name->name !=NULL)
                    {
                        /* iterating over commands in command_lists entry */
                        if (strcmp(register_name->name, field) == 0)
                            return (entry->base_address + j*4);
                        else {
                            register_name = &entry->register_list[++j];
                        }
                    }
                    return 0;
                }
            }
        }
    }

    return 0;
}

uint8_t xcvr_get_state(const ciot25_xcvr_t *dev)
{
    return dev->settings.state;
}

void xcvr_set_state(ciot25_xcvr_t *dev, uint8_t state)
{
#if ENABLE_DEBUG
    switch (state) {
    case XCVR_RF_IDLE:
        DEBUG("[xcvr] Change state: IDLE\n");
        break;
    case XCVR_RF_RX_RUNNING:
        DEBUG("[xcvr] Change state: RX\n");
        break;
    case XCVR_RF_TX_RUNNING:
        DEBUG("[xcvr] Change state: TX\n");
        break;
    default:
        DEBUG("[xcvr] Change state: UNKNOWN\n");
        break;
    }
#endif
    dev->settings.state = state;
}

void xcvr_set_register(ciot25_xcvr_t *dev, sfr_register_t *sfr_register)
{
    uint32_t *sfr_address = _get_addr(sfr_register->type, sfr_register->field);

    if (sfr_address)
    {
        DEBUG("[xcvr] Set register @ %p to the value 0x%04x\n", sfr_address, sfr_register->value);
        *sfr_address = sfr_register->value;
    }
    else
        DEBUG("[xcvr] Register not found\n");
}

void xcvr_get_register(ciot25_xcvr_t *dev, sfr_register_t *sfr_register)
{
    uint32_t *sfr_address = _get_addr(sfr_register->type, sfr_register->field);

    if (sfr_address)
    {
        sfr_register->value = *sfr_address;
        DEBUG("[xcvr] Get register @ %p value 0x%04x\n", sfr_address, sfr_register->value);
    } else
        DEBUG("[xcvr] Register not found\n");
}

uint8_t xcvr_get_syncword_length(const ciot25_xcvr_t *dev)
{
    return dev->settings.fsk.sync_len;
}

void xcvr_set_syncword_length(ciot25_xcvr_t *dev, uint8_t len)
{
    DEBUG("[xcvr] Set syncword length: %d\n", len);
    assert(len < 8);
    assert(len > 0);

    dev->settings.fsk.sync_len = len;

    //  bit 18-16  rw Sync word length
    codec->packet_config = ((codec->packet_config) & XCVR_CODEC_PACKET_CONFIG_SYNC_LEN_MASK) | ((len-1) << 16);
}

uint8_t xcvr_get_syncword(const ciot25_xcvr_t *dev, uint8_t *syncword, uint8_t sync_size)
{
    uint8_t size;

    size = ((codec->packet_config & ~XCVR_CODEC_PACKET_CONFIG_SYNC_LEN_MASK) >> 16) + 1;
    assert(size <= sync_size);

    uint32_t value = 0;
    value = codec->sync_word_l;
    //value = __builtin_bswap32(value);
    memcpy(syncword, &value, size);

    return size;
}

void xcvr_set_syncword(ciot25_xcvr_t *dev, uint8_t *syncword, uint8_t sync_size)
{
    assert(sync_size >= 1);
    assert(sync_size < 8);

     uint32_t *syncword_ptr = (uint32_t *)syncword;

    //syncword is filled MSB first but register is written LSB first (little endian)

    if (sync_size > 4)
    {
        uint32_t mask= 0xFFFFFFFF >> (8 - sync_size)*8;
        codec->sync_word_h = syncword_ptr[1] & mask;
        codec->sync_word_l = syncword_ptr[0];
    }
    else
    {
        uint32_t mask= 0xFFFFFFFF >> (4 - sync_size)*8;
        codec->sync_word_l = syncword_ptr[0] & mask;
    }

    DEBUG("[xcvr] set syncword l: <%08x> :", codec->sync_word_l);
    DEBUG("[xcvr] set syncword h: <%08x> :", codec->sync_word_h);

    xcvr_set_syncword_length(dev, sync_size);

}

static uint32_t compute_base_frequency(uint32_t channel, uint8_t coeff_div)
{
    unsigned long long freq;

    DEBUG("[xcvr] channel %ld %04x\n", channel, channel);

    freq = (unsigned long long)(channel * ((pow(2,24)*coeff_div)/ XCVR_XTAL_FREQ));

    DEBUG("[xcvr] Set base frequency to %08X\n",freq);

    return (uint32_t)freq;
}

uint32_t compute_freq_carrier(uint32_t freq)
{
    uint64_t channel;
    unsigned coef_divider ;

    /* carrier frequency is given by fref * (N + FRAC channel ) / div */


    coef_divider = (radio->rx_cfg & ~XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_MASK) ? 2 : 1;

    uint8_t coef = (uint8_t)(log2(XCVR_XTAL_FREQ));
    DEBUG("[xcvr] coef: %d\n", coef);

    DEBUG("[xcvr] compute freq_carrier from register value: %lu\n", freq);

    if (XCVR_XTAL_FREQ == 32e6) // calculation optimisation when the CPU clock = 32MHz
    {
            channel = (uint64_t)((uint64_t)freq * 15625);
            DEBUG("[xcvr] Set channel frequency Multiplication: %lu\n", channel);
            channel = (channel / (coef_divider * pow(2,14)));
            DEBUG("[xcvr] Set channel frequency with optimisation: %lu\n", channel);
    }
    else
    {
        channel = (uint64_t)((uint64_t)freq * XCVR_XTAL_FREQ);
        channel = (channel / (coef_divider * pow(2,24)));
        DEBUG("[xcvr] Set channel frequency: %lu\n", channel);
    }
    return (uint32_t)channel;
}

unsigned int round_div (int clk, int bitrate) {

    unsigned int clk_div, clk_div_wf,
        clk_div_wof, clk_div_f;

    // clk in kHz
    // bitrate in bps
    // e.g. Round(32000 KHz/55555 bps) = 576

    clk_div_wf  = ((clk*10000)/bitrate);       // Clock div w/ fractional
    clk_div_wof = ((clk*1000)/bitrate);        // Clock div wo/ fractional
    clk_div_f   = clk_div_wf-(clk_div_wof*10); // Clock div fractional
    if (clk_div_f > 5) {
        clk_div = clk_div_wof+1;
    } else {
        clk_div = clk_div_wof;
    }
    return clk_div;
}

uint32_t xcvr_get_channel(const ciot25_xcvr_t *dev)
{
    return compute_freq_carrier(xcvr->freq_carrier);
}

void xcvr_set_channel(ciot25_xcvr_t *dev, uint32_t channel, bool save)
{
    unsigned coef_divider ;
    int32_t XTAL_offset;
    DEBUG("[xcvr] Set channel: %lu\n", channel);

    if (channel > XCVR_RF_MID_BAND_THRESH)
    {
        coef_divider = 2;
        radio->rx_cfg = ((radio->rx_cfg & XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_MASK) |
                         XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_2);
        radio->tx_cfg = ((radio->tx_cfg & XCVR_RADIO_TX_CONFIG_VCO_DIVIDER_MASK) |
                                 XCVR_RADIO_TX_CONFIG_VCO_DIVIDER_2);
    }
    else
    {
        coef_divider = 4;
        radio->rx_cfg = ((radio->rx_cfg & XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_MASK) |
                         XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_4);
        radio->tx_cfg = ((radio->tx_cfg & XCVR_RADIO_TX_CONFIG_VCO_DIVIDER_MASK) |
                                        XCVR_RADIO_TX_CONFIG_VCO_DIVIDER_4);
    }

    /* Read XTAL frequency compensation */
    XTAL_offset = *((volatile unsigned int*)XCVR_FREQ_OFFSET_SET_ADDR);

    DEBUG("[xcvr] XTAL freq offset: %ld", XTAL_offset);

    xcvr->freq_carrier = compute_base_frequency((channel - XTAL_offset), coef_divider);

    DEBUG("[xcvr] Set freq carrier: %08X", xcvr->freq_carrier);

    /* Save current channel */
    if (save)
        dev->settings.channel = channel;
}


void xcvr_set_standby(ciot25_xcvr_t *dev)
{
    if (dev->settings.state == XCVR_RF_IDLE)
    {
        DEBUG("[xcvr] Already in standby\n");
        return;
    }

    DEBUG("[xcvr] Set standby\n");

    /* Disable running timers */
    timer_cancel_event(&dev->_internal.tx_timeout_timer);
    timer_cancel_event(&dev->_internal.rx_timeout_timer);

    // by default, keep only the option to notify the upper layer when a complete frame is received.
    dev->options = XCVR_OPT_TELL_RX_END;

    /* Disable the interrupts */
    xcvr->mask = 0;
    dif->rx_mask = 0;
    dif->tx_mask = 0 ;

    radio->tx_cfg &= ~XCVR_RADIO_TX_CONFIG_TRANSMISSION_ENABLE;
    xcvr_set_op_mode(dev, XCVR_OPMODE_STANDBY );
    xcvr_set_state(dev,  XCVR_RF_IDLE);
}

void xcvr_restart_rx_chain(ciot25_xcvr_t *dev)
{
    // Restart the reception until upper layer decides to stop it
    dev->packet.fifothresh = XCVR_DATA_IF_RX_THRESHOLD;
    dev->packet.length = 0;
    dev->packet.pos = 0;

    DEBUG("restart RX chain with PLL lock");

    // STOP Rssi if not coming from interrupt so RSSI all measurements are reset
    if((baseband->rssi_config & 0x1) == 0x1)
        baseband->rssi_config &= ~0x1;

    // RECEIVER MODE --> so we can flush
    xcvr->op_mode = 0x2;
    //xcvr_set_op_mode(dev, XCVR_OPMODE_RECEIVER);

    xcvr_flush_rx_fifo();

    // STANDBY MODE (IPG) --> so CLK_CDR is enable
    // This allow a lost byte to be written in order to flush it before the real restart
    xcvr->op_mode = 0x0;
//    hw_counter_init(1, 100, false);
//    while(!hw_counter_is_expired(1));

    xtimer_usleep(1000);

    // RECVEIVER MODE --> so we can flush any bytes that are not supposed to be here
    xcvr->op_mode = 0x2;
    //xcvr_set_op_mode(dev, XCVR_OPMODE_RECEIVER);

    xcvr_flush_rx_fifo();

    baseband->rssi_config |= 0x3;

    // wait before enabling the demodulator upon RSSI measurement
//    hw_counter_init(1, 10000, false);
//    while(!hw_counter_is_expired(1));
    xtimer_usleep(10000);

    DEBUG("Nb Preamble : %08x\n", codec->preamble_det_ndet);

    // ENABLE DEMODULATOR ON RSSI
    baseband->rssi_config &= ~0x2;

    DEBUG("XCVR status: %08x\n", xcvr->status);
    DEBUG("DIF  status: %08x\n\n", dif->rx_status);

    xcvr->mask = XCVR_PAYLOAD_READY; // | XCVR_PREAMBLE_DETECTION_IN_PROGRESS;
    //xcvr->mask = ((xcvr->mask) & XCVR_SYNC_TIMEOUT_MASK) | XCVR_SYNC_TIMEOUT;
    /*
    if (dev->options & XCVR_OPT_TELL_RX_END)
    {
        xcvr->mask = ((xcvr->mask) & XCVR_PAYLOAD_READY_MASK) | XCVR_PAYLOAD_READY;
    }*/
}

void xcvr_set_rx(ciot25_xcvr_t *dev)
{
    DEBUG("[xcvr] Set RX\n");

    dev->packet.length = 0;
    dev->packet.pos = 0;
    dev->packet.fifothresh = 0;

    /* Workaround to set frequency to FC instead of FDEV-*/
    xcvr_set_channel(dev, dev->settings.channel + dev->settings.fsk.fdev, false);

    /* wait startup time to take into account the PLL settling time?*/

    xcvr_set_state(dev, XCVR_RF_RX_RUNNING);

    // START RSSI
    baseband->rssi_config |= XCVR_BASEBAND_RSSI_CONFIG_MEASURE_PROCESSING_ON;

    // STOP RSSI so RSSI all measurements are reset
    if((baseband->rssi_config & 0x1) == 0x1)
        baseband->rssi_config &= ~0x1;

    // RECEIVER MODE --> so we can flush
    xcvr->op_mode = 0x02;

    xcvr_flush_rx_fifo();

    // STANDBY MODE (IPG) --> so CLK_CDR is enable
    // This allow a lost byte ton be written in order to flush it before the real restart
    xcvr->op_mode = 0x00;

//    hw_counter_init(1, 100, false);
//    while(!hw_counter_is_expired(1));

    xtimer_usleep(100);

    // RECVEIVER MODE --> so we can flush any bytes that are not supposed to be here
    xcvr->op_mode = 0x02;
    //xcvr_set_op_mode(dev, XCVR_OPMODE_RECEIVER);

    xcvr_flush_rx_fifo();

    baseband->rssi_config |= 0x3;

    // wait before enabling the demodulator upon RSSI measurement
//    hw_counter_init(1, 1000, false);
//    while(!hw_counter_is_expired(1));

    xtimer_usleep(1000);

    // ENABLE DEMODULATOR
    baseband->rssi_config &= ~0x2;

    if (dev->options & XCVR_OPT_TELL_RX_END)
    {
        xcvr->mask = XCVR_PAYLOAD_READY; // | XCVR_PREAMBLE_DETECTION_IN_PROGRESS;
        //xcvr->mask = ((xcvr->mask) & XCVR_SYNC_TIMEOUT_MASK) | XCVR_SYNC_TIMEOUT;
    }
    else
    {
        // when IRQ flag is not set, we ignore received packets
        //dif->rx_mask = XCVR_DATA_IF_RX_ALMOST_FULL;
        xcvr->mask = XCVR_PAYLOAD_READY;
        //xcvr->mask &= XCVR_CRC_VALID_MASK;
        //hw_gpio_disable_interrupt(dev->params.dio0_pin);
        //hw_gpio_disable_interrupt(dev->params.dio1_pin);
    }

    //if(dev->settings.state  == XCVR_RF_RX_RUNNING)
    //    xcvr_restart_rx_chain(dev); // restart when already in RX so PLL can lock when there is a freq change

}

void xcvr_set_tx(ciot25_xcvr_t *dev)
{
    xcvr_set_state(dev, XCVR_RF_TX_RUNNING);
    xcvr_set_op_mode(dev, XCVR_OPMODE_TRANSMITTER );
}

uint8_t xcvr_get_max_payload_len(const ciot25_xcvr_t *dev)
{
    return codec->packet_len;
}

void xcvr_set_max_payload_len(ciot25_xcvr_t *dev, uint16_t maxlen)
{
    DEBUG("[xcvr] Set max payload len: %d\n", maxlen);
    codec->packet_len = maxlen ;

    if (dev->settings.state == XCVR_RF_RX_RUNNING)
        dev->packet.length = maxlen;
}

uint8_t xcvr_get_op_mode(const ciot25_xcvr_t *dev)
{
    return (xcvr->op_mode & ~XCVR_OPMODE_MASK);
}

void xcvr_set_op_mode(const ciot25_xcvr_t *dev, uint8_t op_mode)
{
    assert (op_mode < 8);

    switch(op_mode) {
    case XCVR_OPMODE_STANDBY:
        DEBUG("[xcvr] Set op mode: STANDBY\n");
        break;
    case XCVR_OPMODE_BYPASS_TX_MANUAL_MODULATION:
        DEBUG("[xcvr] Set op mode: BYPASS_TX_MANUEL_MODUL\n");
        break;
    case XCVR_OPMODE_BYPASS_RX_MANUAL_DEMODULATION:
        DEBUG("[xcvr] Set op mode: BYPASS_RX_MANUEL_DEMODUL\n");
        break;
    case XCVR_OPMODE_BYPASS_RX_DIRECT:
        DEBUG("[xcvr] Set op mode: BYPASS_RX_DIRECT\n");
        break;
    case XCVR_OPMODE_BYPASS_TX_FIFO:
        DEBUG("[xcvr] Set op mode: BYPASS_TX_FIFO\n");
        break;
    case XCVR_OPMODE_BYPASS_RX_FIFO:
        DEBUG("[xcvr] Set op mode: BYPASS_RX_FIFO\n");
        break;
    case XCVR_OPMODE_RECEIVER:
        radio->tx_cfg &= ~XCVR_RADIO_TX_CONFIG_TRANSMISSION_ENABLE;
        //baseband->rssi_config |= XCVR_BASEBAND_RSSI_CONFIG_MEASURE_PROCESSING_ON;
        DEBUG("[xcvr] Set op mode: RECEIVER\n");
        break;
    case XCVR_OPMODE_TRANSMITTER:
        DEBUG("[xcvr] Set op mode: TRANSMITTER\n");
        radio->tx_cfg |= XCVR_RADIO_TX_CONFIG_TRANSMISSION_ENABLE;
        break;
    default:
        DEBUG("[xcvr] Set op mode: UNKNOWN (%d)\n", op_mode);
        break;
    }

    /* Replace previous mode value and setup new mode value */
    xcvr->op_mode = (xcvr->op_mode & XCVR_OPMODE_MASK) | op_mode;
}

bool xcvr_get_crc(const ciot25_xcvr_t *dev)
{
    return (bool)((codec->packet_config & XCVR_CODEC_PACKET_CONFIG_CRC_ON) >> 3);
}

void xcvr_set_crc(ciot25_xcvr_t *dev, bool crc)
{
    DEBUG("[xcvr] Set CRC: %d\n", crc);
 /* packet config
    *     bit 3 rw CRC16 Enable
    *           0: Off (default)
    *           1: On */
    codec->packet_config = (codec->packet_config & XCVR_CODEC_PACKET_CONFIG_CRC_MASK) | (crc << 3);
}

uint8_t xcvr_get_payload_length(const ciot25_xcvr_t *dev)
{
    return codec->packet_len;
}

void xcvr_set_payload_length(ciot25_xcvr_t *dev, uint8_t len)
{
    DEBUG("[xcvr] Set payload len: %d\n", len);

    codec->packet_len = len;
}

uint8_t xcvr_get_tx_power(const ciot25_xcvr_t *dev)
{
    return dev->settings.fsk.power;
}

void xcvr_set_tx_power(ciot25_xcvr_t *dev, int8_t power)
{
    DEBUG("[xcvr] Set power: %d\n", power);
    dev->settings.fsk.power = power;

    uint8_t pow_ctl = 0;
    uint8_t i_ctl = 0;

    //TODO calculate the values to set on the TX configuration register
    // determine the relation between pwo_ctl, i_ctl and the power in dBm
    // pow_ctl, ictl = f(power)
/*
    radio->tx_cfg = ((radio->tx_cfg) & XCVR_RADIO_TX_CONFIG_TRANSMISSION_POWER_CTL_MASK) | (pow_ctl << 1);
    radio->tx_cfg = ((radio->tx_cfg) & XCVR_RADIO_TX_CONFIG_TRANSMISSION_I_CTL_MASK) | (i_ctl << 5);
*/
}

uint16_t xcvr_get_preamble_length(const ciot25_xcvr_t *dev)
{
    return dev->settings.fsk.preamble_len;
}

void xcvr_set_preamble_length(ciot25_xcvr_t *dev, uint8_t preamble)
{
    dev->settings.fsk.preamble_len = preamble;

    preamble = preamble * 8; // Bytes to bits
    DEBUG("[xcvr] Set preamble length (symbols): %d\n", preamble);

    /*
     *  bit 15-8  rw Preamble length
     *  0x20 (default)
     **/
    codec->packet_config = ((codec->packet_config) & XCVR_CODEC_PACKET_CONFIG_PREAMBLE_LENGTH_MASK) | (preamble << 8);
}


void xcvr_set_preamble_detector_size(ciot25_xcvr_t *dev, uint8_t size)
{
    DEBUG("[xcvr] Set preamble detector size (symbols): %d\n", size);

    /*
     *  bit 7-1  rw Preamble detector length
     *  0x20 (default)
     **/
    codec->detector_config = ((codec->detector_config) & XCVR_CODEC_PACKET_CONFIG_PREAMBLE_THRESHOLD_MASK) | (size << 1);
    DEBUG("[xcvr] detector config: %04x\n", codec->detector_config);
}

uint8_t xcvr_get_preamble_detector_size(ciot25_xcvr_t *dev)
{
    uint8_t size = (uint8_t)((codec->detector_config & 0xFF) >> 1) ;

    DEBUG("[xcvr] Get preamble detector size (symbols): %d\n", size);
    return (size);
}

void xcvr_set_rx_timeout(ciot25_xcvr_t *dev, uint32_t timeout)
{
    DEBUG("[xcvr] Set RX timeout: %lu\n", timeout);

    dev->settings.fsk.rx_timeout = timeout;
}

void xcvr_set_tx_timeout(ciot25_xcvr_t *dev, uint32_t timeout)
{
    DEBUG("[xcvr] Set TX timeout: %lu\n", timeout);

    dev->settings.fsk.tx_timeout = timeout;
}

uint32_t xcvr_get_bitrate(const ciot25_xcvr_t *dev)
{
    uint32_t br = (xcvr->bitrate) * 100; // Bitrate given in hundreds of bps

    return br ;
}

void xcvr_set_bitrate(ciot25_xcvr_t *dev, uint32_t bps)
{
    uint32_t rate_md;

    DEBUG("[xcvr] Set bitrate register to %i", bps / 100);
    xcvr->bitrate  = (bps / 100); //  hundreds of bps

    if ((bps < 100) || (bps > 200000))
    {
        DEBUG("[xcvr] bitrate out of range %i", bps / 100);
        return;
    }

    if (bps < 200)
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_02_01;
    else if (bps < 400)
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_04_02;
    else if (bps < 800)
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_08_04;
    else if (bps < 1600)
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_16_08;
    else if (bps < 3200)
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_32_16;
    else if (bps < 6400)
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_64_32;
    else if (bps < 12500)
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_125_64;
    else if (bps < 25000)
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_250_125;
    else if (bps < 50000)
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_500_250;
    else if (bps < 100000)
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_1000_500;
    else
        rate_md = XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MODE_2000_1000;

    DEBUG("[xcvr] rate mode to %d", rate_md);
    baseband->rx_config = (baseband->rx_config & XCVR_BASEBAND_RX_CONFIG_DATA_RATE_MASK) | rate_md;

    // set clk divider
    xcvr->clk_divider = round_div(XCVR_FREQ_REF, bps);
}

uint8_t xcvr_get_preamble_polarity(const ciot25_xcvr_t *dev)
{
    uint8_t polarity =  (codec->packet_config & ~XCVR_CODEC_PACKET_CONFIG_PREAMBLE_POLARITY_MASK);
    if (polarity)
        DEBUG("[xcvr] Polarity set to 0x55");
    else
        DEBUG("[xcvr] Polarity set to default  0xAA");
    return (polarity >> 6);
}

void xcvr_set_preamble_polarity(ciot25_xcvr_t *dev, bool polarity)
{
    codec->packet_config = ((codec->packet_config & XCVR_CODEC_PACKET_CONFIG_PREAMBLE_POLARITY_MASK) | (polarity << 6));
    DEBUG("[xcvr] Polarity set to 0x55");
}

void xcvr_set_rssi_threshold(ciot25_xcvr_t *dev, uint8_t rssi_thr)
{
    baseband->rssi_threshold = rssi_thr;
}

uint8_t xcvr_get_rssi_threshold(const ciot25_xcvr_t *dev)
{
    return baseband->rssi_threshold;
}

void xcvr_set_rssi_smoothing(ciot25_xcvr_t *dev, uint8_t rssi_samples)
{
    /*
     * Set the number of averages  = 2 exp (RSSI_naver + 1)
     */
    uint8_t rssi_naver = 0;

    if (rssi_samples < 2)
        rssi_samples = 2;

#ifndef USE_M_LIB
    uint8_t pow2 = 2;
    while (pow2 < rssi_samples)
    {
        pow2 = pow2 << 1;
        rssi_naver++;
    }
#else
    uint8_t rssi_naver = log2(rssi_samples) - 1;
#endif
    baseband->rssi_config = (baseband->rssi_config & XCVR_BASEBAND_RSSI_CONFIG_NUM_AVERAGES_MASK) |
                            rssi_naver << 2;
}

uint8_t xcvr_get_rssi_smoothing(const ciot25_xcvr_t *dev)
{
    uint8_t rssi_naver = (baseband->rssi_config & ~XCVR_BASEBAND_RSSI_CONFIG_NUM_AVERAGES_MASK) >> 2;
    return (1 << (1 + rssi_naver));
}

void xcvr_set_sync_preamble_detect_on(ciot25_xcvr_t *dev, bool enable)
{
    codec->packet_config = (codec->packet_config & XCVR_CODEC_PACKET_CONFIG_SYNC_MODE_MASK) | enable;
}

uint8_t xcvr_get_sync_preamble_detect_on(const ciot25_xcvr_t *dev)
{
    return (codec->packet_config & ~XCVR_CODEC_PACKET_CONFIG_SYNC_MODE_MASK);
}

/*     bit 5 rw PN9 Enable
*           0: Off (default)
*           1: On*/

void xcvr_set_dc_free(ciot25_xcvr_t *dev, uint8_t encoding_scheme)
{
    DEBUG("[xcvr] Set DC free %d", encoding_scheme);
    codec->packet_config = ((codec->packet_config & XCVR_CODEC_PACKET_CONFIG_PN9_MASK) | (encoding_scheme ? 1 : 0) << 5);
}

uint8_t xcvr_get_dc_free(const ciot25_xcvr_t *dev)
{
    if ((codec->packet_config & ~XCVR_CODEC_PACKET_CONFIG_PN9_MASK) >> 5)
        return HW_DC_FREE_WHITENING;
    else
        return HW_DC_FREE_NONE;
}

void xcvr_set_fec_on(ciot25_xcvr_t *dev, bool enable)
{
    DEBUG("[xcvr] Set FEC %d", enable);
    codec->packet_config = (codec->packet_config & XCVR_CODEC_PACKET_CONFIG_FEC_MASK) | (enable << 4);
}

uint8_t xcvr_get_fec_on(const ciot25_xcvr_t *dev)
{
    return (codec->packet_config & ~XCVR_CODEC_PACKET_CONFIG_FEC_MASK);
}

#ifndef USE_M_LIB
static uint32_t computefdev(uint8_t mantissa, uint8_t exponent)
{
    uint32_t vco_div = (radio->rx_cfg & ~XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_MASK) ? 4 : 2;

    return (uint32_t)(((XCVR_XTAL_FREQ)/(2 << 17)) * (XCVR_OFFSET_FREQ_DEV + mantissa) * (2 << exponent) / vco_div);
}
#endif

void computeFdevMantExp(uint32_t fdev, uint8_t* mantisse, uint8_t* exponent)
{

#ifndef USE_M_LIB
    uint32_t tmpfdev;
    uint8_t tmpExp, tmpMant;
    uint32_t fdev_min = 0;

    for( tmpMant = 1; tmpMant <= 8; tmpMant ++ ){
    	//DEBUG("[xcvr] computeFdevMantExp Mantissa %d", tmpMant);
    	for( tmpExp = 0; tmpExp < 8; tmpExp++ ) {
    		//DEBUG("[xcvr] computeFdevMantExp Exponent %d", tmpExp);
    		tmpfdev = computefdev(tmpMant, tmpExp);
    		//DEBUG("[xcvr] computeFdevMantExp result %i, diff %d", tmpfdev, abs( tmpfdev - fdev ));
    		if((abs( tmpfdev - fdev ) < fdev_min ) || (fdev_min == 0)) {
                fdev_min = abs( tmpfdev - fdev );
                *mantisse = tmpMant;
                *exponent = tmpExp;
                if (fdev_min == 0)
                	break;
            }
        }
    }
#else
    uint8_t vco_div = (radio->rx_cfg & ~XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_MASK) ? 4 : 2;

    *exponent = floor(log2(fdev*vco_div*pow(2,17)/(XCVR_OFFSET_FREQ_DEV*XCVR_XTAL_FREQ)));
    *mantisse = round((fdev*vco_div*pow(2,17)/(XCVR_XTAL_FREQ*pow(2,(double)*exponent)))-XCVR_OFFSET_FREQ_DEV);
#endif
}

void xcvr_set_tx_fdev(ciot25_xcvr_t *dev, uint32_t fdev)
{
    uint8_t mantissa, exponent;
    computeFdevMantExp(fdev, &mantissa, &exponent);
    /*Frequency Deviation, rw
       *  bits 23:16   Offset
       *  bits 15:8    Exponent
       *  bits  7:0    Mantissa */
    DEBUG("[xcvr] Set Frequency deviation offset %d Exponent %d Mantissa %d", XCVR_OFFSET_FREQ_DEV, exponent, mantissa);
    uint32_t tmpFdev = (XCVR_OFFSET_FREQ_DEV << 16 ) | (exponent << 8) | mantissa;
    xcvr->freq_dev = tmpFdev ;
    DEBUG("[xcvr] Set Frequency deviation to 0x%lx",tmpFdev );

    dev->settings.fsk.fdev = fdev;
}

uint32_t xcvr_get_tx_fdev(const ciot25_xcvr_t *dev)
{
    uint32_t fdev = xcvr->freq_dev;
    uint8_t mantissa = (uint8_t)fdev;
    uint8_t exponent = (uint8_t)(fdev >> 8);
    uint8_t vco_div = (radio->rx_cfg & ~XCVR_RADIO_RX_CONFIG_VCO_DIVIDER_MASK) ? 4 : 2;

    DEBUG("[xcvr] Get Frequency deviation offset %d Exponent %d Mantissa %d", XCVR_OFFSET_FREQ_DEV, exponent, mantissa);
    // fdev = (fosc / 2e17) * ( 8 + deviation_m) * 2eExponent
    return computefdev(mantissa, exponent);
}

void xcvr_set_modulation_shaping(ciot25_xcvr_t *dev, uint8_t shaping)
{
    /* 
     * bit 3 modulation type
     * 0 =   FSK (no shaping)
     * 1 =   GFSK with Gaussian filter B=0.5
     */
    DEBUG("[xcvr] modulation_shaping set to %d", shaping);

    if (shaping){
        xcvr->clkgen_en = XCVR_CLKGEN_DIS;
        xcvr->prescaler = XCVR_GFSK_PRESCALER;
        xcvr->clkgen_en = XCVR_CLKGEN_EN;
        xcvr->op_mode = (xcvr->op_mode & XCVR_OPMODE_MODULATION_TYPE_MASK) | XCVR_OPMODE_MODULATION_TYPE_GFSK;
    }
    else{
        xcvr->clkgen_en = XCVR_CLKGEN_DIS;
        xcvr->prescaler = XCVR_FSK_PRESCALER;
        xcvr->clkgen_en = XCVR_CLKGEN_EN;
        xcvr->op_mode = (xcvr->op_mode & XCVR_OPMODE_MODULATION_TYPE_MASK) | XCVR_OPMODE_MODULATION_TYPE_2FSK;
    }
}

uint8_t xcvr_get_modulation_shaping(const ciot25_xcvr_t *dev)
{
    uint8_t modulation_shaping = ((xcvr->op_mode) & ~XCVR_OPMODE_MODULATION_TYPE_MASK) >> 3 ;

    if (modulation_shaping)
        DEBUG("Gaussian filter BT = 0.5");
    else
        DEBUG("No shaping");

    return (modulation_shaping);
}

void xcvr_set_pll(ciot25_xcvr_t *dev, bool enable)
{
    if(dev->settings.state != XCVR_RF_TX_RUNNING){
        if(enable)
            xcvr->freq_dev = 0x00000000;

        DEBUG("[xcvr] Set PLL_LOCK: %d\n", enable);
        radio->tx_cfg = (radio->tx_cfg & XCVR_RADIO_TX_CONFIG_TRANSMISSION_MASK) | enable;
    }
    else
        DEBUG("[xcvr] Can't lock pll because in TX state");
}

int xcvr_set_option(ciot25_xcvr_t *dev, uint8_t option, bool state)
{
    switch(option) {
        case XCVR_OPT_TELL_TX_START:
            DEBUG("[xcvr] set_option: inform when TX START (%s)\n", state ? "Enable" : "Disable");
            break;
        case XCVR_OPT_TELL_TX_END:
            DEBUG("[xcvr] set_option: inform when TX is terminated (%s)\n", state ? "Enable" : "Disable");
            break;
        case XCVR_OPT_TELL_RX_START:
            DEBUG("[xcvr] set_option: inform when a packet header is received (%s)\n", state ? "Enable" : "Disable");
            break;
        case XCVR_OPT_TELL_RX_END:
            DEBUG("[xcvr] set_option: inform when a packet is received (%s)\n", state ? "Enable" : "Disable");
            break;
        case XCVR_OPT_TELL_TX_REFILL:
            DEBUG("[xcvr] set_option: inform when TX fifo needs to be refilled (%s)\n", state ? "Enable" : "Disable");
            break;
        case XCVR_OPT_PRELOADING:
            DEBUG("[xcvr] set_option: TX FIFO is just filled for preloading (No TX mode) (%s)\n", state ? "Enable" : "Disable");
            break;
    }

    /* set option field */
    if (state) {
        dev->options |= option;
    }
    else {
        dev->options &= ~(option);
    }

    return sizeof(netopt_enable_t);
}
