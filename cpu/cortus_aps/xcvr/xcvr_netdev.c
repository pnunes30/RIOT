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
 * @brief       Netdev adaptation for the ciot25 xcvr driver
 *
 * @author
 *
 * @}
 */

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "periph_cpu.h"

#include "hwradio.h"
#include "hwcounter.h"
#include "hwsystem.h"
#include "debug.h"

//#include "machine/xcvr.h"
//#include "machine/ic.h"

#include "xcvr.h"
#include "xcvr_internal.h"
#include "xcvr_registers.h"
#include "xcvr_netdev.h"

#include "iolist.h"
#include "net/netdev.h"
#include "net/netopt.h"

#define ENABLE_DEBUG (0)

#if ENABLE_DEBUG
static void log_print_data(uint8_t* message, uint32_t length);
    #define DEBUG_DATA(...) log_print_data(__VA_ARGS__)
#else
    #define DEBUG_DATA(...)
#endif

#if (ENABLE_DEBUG)
static void log_print_data(uint8_t* message, uint32_t length)
{
    for( uint32_t i=0 ; i<length ; i++ )
    {
        printf(" %02X", message[i]);
    }
}
#endif

/* Internal helper functions */
static int _set_state(ciot25_xcvr_t *dev, netopt_state_t state);
static int _get_state(ciot25_xcvr_t *dev, void *val);
void _on_xcvr_rx_interrupt(void *arg);
void _on_xcvr_tx_interrupt(void *arg);
void _on_data_if_rx_interrupt(void *arg);
void _on_data_if_tx_interrupt(void *arg);

bool init_done = false;

/* Netdev driver api functions */
static int _send(netdev_t *netdev, const iolist_t *iolist);
static int _recv(netdev_t *netdev, void *buf, size_t len, void *info);
static int _init(netdev_t *netdev);
static void _isr(netdev_t *netdev);
static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len);
static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len);

static int8_t rssi;

struct xcvr_handle {
    ciot25_xcvr_t ciot25_xcvr;
};

const netdev_driver_t xcvr_driver = {
    .send = _send,
    .recv = _recv,
    .init = _init,
    .isr = _isr,
    .get = _get,
    .set = _set,
};

xcvr_handle_t xcvr_ressource = { .ciot25_xcvr.netdev.driver = &xcvr_driver};

static void dump_register(void)
{

    DEBUG("************************DUMP REGISTER*********************");

    DEBUG("\r\nXCRV register");
    for (uint32_t *add = SFRADR_XCVR; add <= SFRADR_XCVR+0x2c; add++)
        DEBUG("XCVR ADDR %2X DATA %08X", add, *add);

    DEBUG("\r\nSFRADR_CODEC register");
    for (uint32_t *add = SFRADR_CODEC; add <= SFRADR_CODEC + 0x24; add++)
        DEBUG("CODEC ADDR %2X DATA %08X", add, *add);

    DEBUG("\r\nSFRADR_BASEBAND register");
    for (uint32_t *add = SFRADR_BASEBAND; add <= SFRADR_BASEBAND + 0x90; add++)
        DEBUG("BASEBAND ADDR %2X DATA %08X", add, *add);

    DEBUG("\r\nSFRADR_RADIO register");
    for (uint32_t *add = SFRADR_RADIO; add <= SFRADR_RADIO + 0x14; add++)
        DEBUG("RADIO ADDR %2X DATA %08X", add, *add);

    DEBUG("\r\nSFRADR_DATA_IF register");
     for (uint32_t *add = SFRADR_DATA_IF; add <= SFRADR_DATA_IF + 0x24; add++)
        DEBUG("DATA_IF ADDR %2X DATA %08X", add, *add);


    // Please note that when reading the first byte of the FIFO register, this
    // byte is removed so the dump is not recommended before a TX or take care
    // to fill it after the dump

    DEBUG("**********************************************************");
}




static int _send(netdev_t *netdev, const iolist_t *iolist)
{
    ciot25_xcvr_t *dev = (ciot25_xcvr_t*) netdev;

    // assume only one item in this list
    uint8_t size = iolist->iol_len; //iolist_size(iolist);

    if (xcvr_get_state(dev) == XCVR_RF_TX_RUNNING)
    {
        // Check if refill is expected
        if (dev->options & XCVR_OPT_TELL_TX_REFILL)
        {
            /* refill the payload buffer */
            memcpy((void*)dev->packet.buf, iolist->iol_base, size);
            dev->packet.pos = 0;
            dev->packet.length = size;
            return 0;
        }
        else
        {
            DEBUG("[xcvr] [WARNING] Cannot send packet: radio already in transmitting "
                  "state.\n");
            return -ENOTSUP;
        }
    }

    /* Assume the chip is in idle, no need to wake up the peripheral if the
     * main CPU is active */

    dif->tx_thr = XCVR_DATA_IF_TX_THRESHOLD;
    //xcvr_flush_tx_fifo();

    dev->packet.length = size;

    codec->packet_config = (codec->packet_config & XCVR_CODEC_PACKET_CONFIG_LEN_MASK) | XCVR_CODEC_PACKET_CONFIG_LEN_FIX;

    if (size > XCVR_FIFO_MAX_SIZE)
    {
        dif->tx_thr = XCVR_DATA_IF_TX_THRESHOLD;
        dev->packet.fifothresh = XCVR_DATA_IF_TX_THRESHOLD;

        memcpy((void*)dev->packet.buf, iolist->iol_base, size);
        /* Write payload buffer */
        xcvr_write_fifo(dev, dev->packet.buf, XCVR_FIFO_MAX_SIZE);

        dev->packet.pos = XCVR_FIFO_MAX_SIZE;
        dif->tx_mask = XCVR_DATA_IF_TX_ALMOST_EMPTY;
    }
    else
    {
        dev->packet.pos = size;
        if (dev->options & XCVR_OPT_TELL_TX_REFILL) // we expect to refill the FIFO with subsequent data
        {
            dif->tx_thr = 20; // FIFO level interrupt if under 20 bytes
            dev->packet.fifothresh = 20;
            codec->packet_config = (codec->packet_config & XCVR_CODEC_PACKET_CONFIG_LEN_MASK) | XCVR_CODEC_PACKET_CONFIG_LEN_INFI;
            xcvr_write_fifo(dev, iolist->iol_base, size);

            dif->tx_mask = XCVR_DATA_IF_TX_ALMOST_EMPTY;

        }
        else
        {
            xcvr_write_fifo(dev, iolist->iol_base, size);
            xcvr->mask = (xcvr->mask & XCVR_TX_DONE_MASK) | XCVR_TX_DONE_ENABLE;
        }
    }

    if (!(dev->options & XCVR_OPT_PRELOADING))
    {
        /* Put chip into transfer mode */
        xcvr_set_state(dev, XCVR_RF_TX_RUNNING);
        xcvr_set_op_mode(dev, XCVR_OPMODE_TRANSMITTER);
    }

    return 0;
}

static int _recv(netdev_t *netdev, void *buf, size_t len, void *info)
{
    ciot25_xcvr_t *dev = (ciot25_xcvr_t*) netdev;

    /* just return length when buf == NULL */
    if (buf == NULL) {
        return dev->packet.length;
    }

    if (dev->packet.length > len) {
        return -ENOSPC;
    }

    memcpy(buf, (void*)dev->packet.buf, dev->packet.length);
    if (info != NULL) {
        netdev_xcvr_packet_info_t *packet_info = info;

        packet_info->rssi = rssi; //(int8_t)baseband->rssi_latch_mean_val;
        packet_info->lqi = 0;
        packet_info->crc_status = dev->packet.crc_status;
    }
    return dev->packet.length;
}

static int _init(netdev_t *netdev)
{
    ciot25_xcvr_t *ciot25_xcvr = (ciot25_xcvr_t*)netdev;

    if (init_done)
        return 0;

    ciot25_xcvr->irq_flags = 0;
    xcvr_radio_settings_t settings;
    settings.channel = XCVR_CHANNEL_DEFAULT;
    settings.state = XCVR_RF_IDLE;

    ciot25_xcvr->settings = settings;

    /* Launch initialization of driver and device */
    DEBUG("[xcvr] netdev: initializing driver...\n");
    if (xcvr_init(ciot25_xcvr) != XCVR_INIT_OK) {
        DEBUG("[xcvr] netdev: initialization failed\n");
        return -1;
    }

    xcvr_init_radio_settings(ciot25_xcvr);

    /* Put chip into idle mode */
    xcvr_set_standby(ciot25_xcvr);

    // enable interrupt for the XCVR status and the FIFO level
    aps_irq[IRQ_XCVR_TX].ipl = 0;
    aps_irq[IRQ_XCVR_TX].ien = 1;

    aps_irq[IRQ_XCVR_RX].ipl = 0;
    aps_irq[IRQ_XCVR_RX].ien = 1;

    aps_irq[IRQ_DATA_IF_TX].ipl = 0;
    aps_irq[IRQ_DATA_IF_TX].ien = 1;

    //aps_irq[IRQ_DATA_IF_RX].ipl = 0;
    //aps_irq[IRQ_DATA_IF_RX].ien = 1;

    DEBUG("[xcvr] init_radio: xcvr initialization done\n");
    init_done = true;

    return 0;
}

static void _isr(netdev_t *netdev)
{
    ciot25_xcvr_t *dev = (ciot25_xcvr_t *) netdev;

    uint8_t irq_flags = dev->irq_flags;
    dev->irq_flags = 0;

    switch (irq_flags) {
        case IRQ_DATA_IF_TX:
            _on_data_if_tx_interrupt(dev);
            break;

        case IRQ_DATA_IF_RX:
            _on_data_if_rx_interrupt(dev);
            break;

        case IRQ_XCVR_TX:
            _on_xcvr_tx_interrupt(dev);
            break;

        case IRQ_XCVR_RX:
            _on_xcvr_rx_interrupt(dev);
            break;

        default:
            break;
    }
}

static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len)
{
    ciot25_xcvr_t *dev = (ciot25_xcvr_t*) netdev;

    if (dev == NULL) {
        return -ENODEV;
    }

    switch(opt) {
        case NETOPT_STATE:
            assert(max_len >= sizeof(netopt_state_t));
            return _get_state(dev, val);

        case NETOPT_CHANNEL_FREQUENCY:
            assert(max_len >= sizeof(uint32_t));
            *((uint32_t*) val) = xcvr_get_channel(dev);
            return sizeof(uint32_t);

        case NETOPT_BANDWIDTH:
            assert(max_len >= sizeof(uint32_t));
            // NOT supported by CIOT25 XCVR
            //*((uint32_t*) val) = xcvr_get_bandwidth(dev);
            return sizeof(uint32_t);

        case NETOPT_BITRATE:
            assert(max_len >= sizeof(uint32_t));
            *((uint32_t*) val) = xcvr_get_bitrate(dev);
            return sizeof(uint32_t);

        case NETOPT_MAX_PACKET_SIZE:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_max_payload_len(dev);
            return sizeof(uint8_t);

        case NETOPT_INTEGRITY_CHECK:
            assert(max_len >= sizeof(netopt_enable_t));
            *((netopt_enable_t*) val) = xcvr_get_crc(dev) ? NETOPT_ENABLE : NETOPT_DISABLE;
            return sizeof(netopt_enable_t);

        case NETOPT_TX_POWER:
            assert(max_len >= sizeof(int8_t));
            *((int8_t*) val) = (int8_t)xcvr_get_tx_power(dev);
            return sizeof(int8_t);

        case NETOPT_SYNC_LENGTH:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_syncword_length(dev);
            return sizeof(uint8_t);

        case NETOPT_SYNC_WORD:
            return xcvr_get_syncword(dev, (uint8_t*)val, max_len);

        case NETOPT_FDEV:
            assert(max_len >= sizeof(uint32_t));
            *((uint32_t*) val) = xcvr_get_tx_fdev(dev);
            return sizeof(uint32_t);

        case NETOPT_MOD_SHAPING:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_modulation_shaping(dev);
            return sizeof(uint8_t);

        case NETOPT_CCA_THRESHOLD:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_rssi_threshold(dev);
            return sizeof(uint8_t);

        case NETOPT_RSSI_SMOOTHING:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_rssi_smoothing(dev);
            return sizeof(uint8_t);

        case NETOPT_SYNC_ON:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_sync_preamble_detect_on(dev);
            return sizeof(uint8_t);

        case NETOPT_PREAMBLE_DETECT_ON:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_sync_preamble_detect_on(dev);
            return sizeof(uint8_t);

        case NETOPT_PREAMBLE_LENGTH:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_preamble_length(dev);
            return sizeof(uint8_t);

        case NETOPT_PREAMBLE_POLARITY:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_preamble_polarity(dev);
            return sizeof(uint8_t);

        case NETOPT_DC_FREE_SCHEME:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_dc_free(dev);
            return sizeof(uint8_t);

        case NETOPT_FEC_ON:
            assert(max_len >= sizeof(uint8_t));
            *((uint8_t*) val) = xcvr_get_fec_on(dev);
            return sizeof(uint8_t);

        case NETOPT_RSSI_VALUE:
            assert(max_len >= sizeof(int16_t));
            *((int16_t*) val) = xcvr_read_rssi(dev);
            return sizeof(int16_t);

        case NETOPT_PRELOADING:
            if (dev->options & XCVR_OPT_PRELOADING) {
                *((netopt_enable_t *)val) = NETOPT_ENABLE;
            }
            else {
                *((netopt_enable_t *)val) = NETOPT_DISABLE;
            }
            return sizeof(netopt_enable_t);

        case NETOPT_XCVR_REGISTER:
            xcvr_get_register(dev, (sfr_register_t*)val);
            return sizeof(sfr_register_t);

        default:
            break;
    }

    return -ENOTSUP;
}

static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len)
{
    ciot25_xcvr_t *dev = (ciot25_xcvr_t*) netdev;
    int res = -ENOTSUP;

    if (dev == NULL) {
        return -ENODEV;
    }

    switch(opt) {
        case NETOPT_STATE:
            assert(len <= sizeof(netopt_state_t));
            return _set_state(dev, *((const netopt_state_t*) val));

        case NETOPT_CHANNEL_FREQUENCY:
            assert(len <= sizeof(uint32_t));
            xcvr_set_channel(dev, *((const uint32_t*) val));
            return sizeof(uint32_t);

        case NETOPT_BITRATE:
            assert(len <= sizeof(uint32_t));
            xcvr_set_bitrate(dev, *((const uint32_t*) val));
            return sizeof(uint32_t);

        case NETOPT_MAX_PACKET_SIZE:
            assert(len <= sizeof(uint16_t));
            xcvr_set_max_payload_len(dev, *((const uint16_t*) val));
            return sizeof(uint16_t);

        case NETOPT_INTEGRITY_CHECK:
            assert(len <= sizeof(netopt_enable_t));
            xcvr_set_crc(dev, *((const netopt_enable_t*) val) ? true : false);
            return sizeof(netopt_enable_t);

        case NETOPT_RX_TIMEOUT:
            assert(len <= sizeof(uint32_t));
            xcvr_set_rx_timeout(dev, *((const uint32_t*) val));
            return sizeof(uint32_t);

        case NETOPT_TX_POWER:
            assert(len <= sizeof(int8_t));
            int8_t power = *((const int8_t *)val);
            if ((power < INT8_MIN) || (power > INT8_MAX)) {
                res = -EINVAL;
                break;
            }
            xcvr_set_tx_power(dev, (int8_t)power);
            return sizeof(int16_t);

        case NETOPT_PREAMBLE_LENGTH:
            assert(len <= sizeof(uint8_t));
            xcvr_set_preamble_length(dev, *((const uint8_t*) val));
            return sizeof(uint8_t);

        case NETOPT_PREAMBLE_POLARITY:
            assert(len <= sizeof(uint8_t));
            xcvr_set_preamble_polarity(dev, *((const uint8_t*) val));
            return sizeof(uint8_t);

        case NETOPT_PREAMBLE_DETECT_ON:
            assert(len <= sizeof(uint8_t));
            xcvr_set_sync_preamble_detect_on(dev, *((const uint8_t*) val));
            return sizeof(uint8_t);

        case NETOPT_SYNC_ON:
            assert(len <= sizeof(uint8_t));
            xcvr_set_sync_preamble_detect_on(dev, *((const uint8_t*) val));
            return sizeof(uint8_t);

        case NETOPT_SYNC_LENGTH:
            assert(len <= sizeof(uint8_t));
            xcvr_set_syncword_length(dev, *((const uint8_t*) val));
            return sizeof(uint8_t);

        case NETOPT_SYNC_WORD:
            xcvr_set_syncword(dev, (uint8_t*)val, len);
            return len;

        case NETOPT_FDEV:
            assert(len <= sizeof(uint32_t));
            xcvr_set_tx_fdev(dev, *((const uint32_t*) val));
            return sizeof(uint32_t);

        case NETOPT_CCA_THRESHOLD:
            assert(len <= sizeof(uint8_t));
            xcvr_set_rssi_threshold(dev, *((const uint8_t*) val));
            return sizeof(uint8_t);

        case NETOPT_MOD_SHAPING:
            assert(len <= sizeof(uint8_t));
            xcvr_set_modulation_shaping(dev, *((const uint8_t*) val));
            return sizeof(uint8_t);

        case NETOPT_RSSI_SMOOTHING:
            assert(len <= sizeof(uint8_t));
            xcvr_set_rssi_smoothing(dev, *((const uint8_t*) val));
            return sizeof(uint8_t);

        case NETOPT_DC_FREE_SCHEME:
            assert(len <= sizeof(uint8_t));
            xcvr_set_dc_free(dev, *((const uint8_t*) val));
            return sizeof(uint8_t);

        case NETOPT_FEC_ON:
            assert(len <= sizeof(uint8_t));
            xcvr_set_fec_on(dev, *((const uint8_t*) val));
            return sizeof(uint8_t);

        case NETOPT_RX_START_IRQ:
            return xcvr_set_option(dev, XCVR_OPT_TELL_RX_START, *((const bool *)val));

        case NETOPT_RX_END_IRQ:
            return xcvr_set_option(dev, XCVR_OPT_TELL_RX_END, *((const bool *)val));

        case NETOPT_TX_START_IRQ:
            return xcvr_set_option(dev, XCVR_OPT_TELL_TX_START, *((const bool *)val));

        case NETOPT_TX_END_IRQ:
            return xcvr_set_option(dev, XCVR_OPT_TELL_TX_END, *((const bool *)val));

        case NETOPT_TX_REFILL_IRQ:
            return xcvr_set_option(dev, XCVR_OPT_TELL_TX_REFILL, *((const bool *)val));

        case NETOPT_PRELOADING:
            return xcvr_set_option(dev, XCVR_OPT_PRELOADING, *((const bool *)val));

        case NETOPT_XCVR_REGISTER:
            xcvr_set_register(dev, (sfr_register_t*)val);
            return len;

        case NETOPT_PLL_LOCK:
            xcvr_set_pll(dev, *((const bool *)val));
            return len;

        default:
            break;
    }

    return res;
}

static int _set_state(ciot25_xcvr_t *dev, netopt_state_t state)
{
    switch (state) {
        case NETOPT_STATE_STANDBY:
            xcvr_set_standby(dev);
            break;
        case NETOPT_STATE_RX:
            radio->tx_cfg &= ~XCVR_RADIO_TX_CONFIG_TRANSMISSION_ENABLE;
            xcvr_set_rx(dev);
            //dump_register();
            //xcvr->mask |= 0x08;
            break;
        case NETOPT_STATE_TX:
            xcvr_set_tx(dev);
            break;
        case NETOPT_STATE_BYPASS_TX_MANUAL_MODULATION:
        case NETOPT_STATE_BYPASS_RX_MANUAL_DEMODULATION:
        case NETOPT_STATE_RX_DIRECT:
        case NETOPT_STATE_TX_FIFO:
        case NETOPT_STATE_RX_FIFO:
        default:
            return -ENOTSUP;
    }
    return sizeof(netopt_state_t);
}

static int _get_state(ciot25_xcvr_t *dev, void *val)
{
    netopt_state_t state = (netopt_state_t)xcvr_get_op_mode(dev);

    memcpy(val, &state, sizeof(netopt_state_t));
    return sizeof(netopt_state_t);
}

static void fill_in_fifo(ciot25_xcvr_t *dev)
{
    uint8_t remaining_bytes = dev->packet.length - dev->packet.pos;
    uint8_t space_left = XCVR_FIFO_MAX_SIZE - dev->packet.fifothresh;

    if (remaining_bytes == 0) // means that we need to ask the upper layer to refill the fifo
    {
        netdev_t *netdev = (netdev_t*) &dev->netdev;
        netdev->event_callback(netdev, NETDEV_EVENT_TX_REFILL_NEEDED);

        //update remaining_bytes
        remaining_bytes = dev->packet.length - dev->packet.pos;
        if (remaining_bytes == 0) // no new data, end of transmission
        {
            xcvr->mask = (xcvr->mask & XCVR_TX_DONE_MASK) | XCVR_TX_DONE_ENABLE;
            return;
        }
    }

    if (remaining_bytes > space_left)
    {
        dev->packet.pos += space_left;

        // clear the interrupt
        dif->tx_thr = XCVR_DATA_IF_TX_THRESHOLD;
        dev->packet.fifothresh = XCVR_DATA_IF_TX_THRESHOLD;
        xcvr_write_fifo(dev, &dev->packet.buf[dev->packet.pos], space_left);

        dev->packet.pos += space_left;

        // set the interrupt
        dif->tx_mask |=  XCVR_DATA_IF_TX_ALMOST_EMPTY;
    }
    else
    {
        xcvr_write_fifo(dev, &dev->packet.buf[dev->packet.pos], remaining_bytes);
        dev->packet.pos += remaining_bytes;

        if (dev->options & XCVR_OPT_TELL_TX_REFILL)
        {
            dif->tx_mask = (dif->tx_mask & ~XCVR_DATA_IF_TX_ALMOST_EMPTY) | XCVR_DATA_IF_TX_ALMOST_EMPTY;
        }
        else
        {
            xcvr->mask = (xcvr->mask & XCVR_TX_DONE_MASK) | XCVR_TX_DONE_ENABLE;
        }
    }
}

/**
 * IRQ handlers
 */
void xcvr_isr(netdev_t *dev, xcvr_flags_t flag)
{
    ciot25_xcvr_t *ciot25_xcvr = (ciot25_xcvr_t*)dev;

    ciot25_xcvr->irq_flags |= flag;

    if (dev->event_callback) {
        dev->event_callback(dev, NETDEV_EVENT_ISR);
    }
}

void interrupt_handler(IRQ_DATA_IF_TX)
{
    __enter_isr();
    ciot25_xcvr_t *dev = &xcvr_ressource.ciot25_xcvr;

    dif->tx_mask &= ~XCVR_DATA_IF_TX_ALMOST_EMPTY;

    if (dif->tx_status & XCVR_DATA_IF_TX_ALMOST_EMPTY){
            fill_in_fifo(dev);
    }
    else
        DEBUG("[xcvr] tx_status = %02x", dif->tx_status);

    //xcvr_isr((netdev_t *)dev, IRQ_DATA_IF_TX);
    __exit_isr();
}

void interrupt_handler(IRQ_DATA_IF_RX)
{
    __enter_isr();
    ciot25_xcvr_t *dev = &xcvr_ressource.ciot25_xcvr;

    //DEBUG("[xcvr] IRQ_DATA_IF_RX rx_status %08x", dif->rx_status);

    dif->rx_mask &= ~XCVR_DATA_IF_RX_ALMOST_FULL;

    xcvr_isr((netdev_t *)dev, IRQ_DATA_IF_RX);
    __exit_isr();
}

void interrupt_handler(IRQ_XCVR_TX)
{
    __enter_isr();
    ciot25_xcvr_t *dev = &xcvr_ressource.ciot25_xcvr;

	assert(dev->settings.state == XCVR_RF_TX_RUNNING);

    xcvr->mask = (xcvr->mask & XCVR_TX_DONE_MASK);

    timer_cancel_event(&dev->_internal.tx_timeout_timer);

    xcvr_isr((netdev_t *)dev, IRQ_XCVR_TX);
    __exit_isr();
}

void interrupt_handler(IRQ_XCVR_RX)
{
    __enter_isr();
    ciot25_xcvr_t *dev = &xcvr_ressource.ciot25_xcvr;

    uint32_t status;

    if (xcvr->status & XCVR_PREAMBLE_DETECTION_IN_PROGRESS)
        DEBUG("[xcvr] PRD in progress : %08x\n", xcvr->status);

    if (!(xcvr->status & XCVR_PAYLOAD_READY))
         return;

    xcvr->mask &= XCVR_PAYLOAD_READY_MASK;
    xcvr->mask &= XCVR_PREAMBLE_DETECTION_IN_PROGRESS_MASK;
    xcvr->mask &= XCVR_SYNC_TIMEOUT_MASK;
    //assert(xcvr->status & XCVR_PAYLOAD_READY);
    rssi = (int8_t)baseband->rssi_latch_mean_val;
    DEBUG("[xcvr] XCVR status: %08x\n", xcvr->status);
    DEBUG("[xcvr] DIF  status: %08x\n", dif->rx_status);

    dev->packet.crc_status = HW_CRC_UNAVAILABLE;
    // read the first byte if the packet length is not set
    if (dev->packet.length == 0)
        dev->packet.buf[dev->packet.pos++] = dif->rx_data;

    dev->packet.length = dev->packet.buf[0] + 1;

    // Limitation until the interrupt ALMOST FULL is enabled
    if(dev->packet.length > XCVR_FIFO_MAX_SIZE)
    	dev->packet.length = XCVR_FIFO_MAX_SIZE;

    xcvr_read_fifo(dev, &dev->packet.buf[dev->packet.pos], dev->packet.length - dev->packet.pos);

    xcvr_isr((netdev_t *)dev, IRQ_XCVR_RX);
    __exit_isr();
}

void _on_xcvr_tx_interrupt(void *arg)
{
    /* Get interrupt context */
    ciot25_xcvr_t *dev = (ciot25_xcvr_t *) arg;
    netdev_t *netdev = (netdev_t*) &dev->netdev;

    //timer_cancel_event(&dev->_internal.tx_timeout_timer);

    if (xcvr->status & XCVR_TX_DONE_ENABLE ){
        xcvr_set_standby(dev);
        xcvr_set_state(dev, XCVR_RF_IDLE);
        netdev->event_callback(netdev, NETDEV_EVENT_TX_COMPLETE);
    }
}

void _on_xcvr_rx_interrupt(void *arg)
{
    /* Get interrupt context */
    ciot25_xcvr_t *dev = (ciot25_xcvr_t *) arg;
    netdev_t *netdev = (netdev_t*) &dev->netdev;

    //assert(xcvr->status & XCVR_PAYLOAD_READY);

    DEBUG("[xcvr] AFC offset: %08x\n", baseband->afc_offs);

    // STOP DEMODULATION PROCEDURE
    baseband->dm_config |= 0x1;
    // Stop RSSI
    baseband->rssi_config &= ~0x1;
    // WARNING: the CODEC can receive a new packed when entering IPG.
    // BUT it MUST have a cdr_clk cycle to validate its status, so keep the demodulator
    // working for a while:
    xcvr->op_mode = 0x00;

    //hw_counter_init(1, 700, false);
    //while(!hw_counter_is_expired(1));
    xtimer_usleep(700);

    // WARNING: Stop demodulation - this MUST come after RSSI is disabled
    baseband->dm_config &= ~0x01;
    // END STOP DEMODULATION PROCEDURE

    DEBUG("[xcvr] Nb Preamble : %08x\n", codec->preamble_det_ndet);

    //if((dif->rx_status & 0x4) != 0x4)
    netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
    // procedure to restart the RX
    xcvr_restart_rx_chain(dev);
}

void _on_data_if_rx_interrupt(void *arg)
{
    /* Get interrupt context */
    ciot25_xcvr_t *dev = (ciot25_xcvr_t *) arg;
    netdev_t *netdev = (netdev_t*) dev;

/*
    while ((dif->rx_status & 0x1) != 0){ // Check if FIFO is empty
      if (!got_data_rx_len){
        got_data_rx_len = 1;
        data_rx_len = dif->rx_data;
        i = 0;
        dev->packet.buf[0] = data_rx_len;
      } else {
        i++;
        dev->packet.buf[i] = dif->rx_data;
      }
    }
    if (i == data_rx_len) {
      data_rx_status = dif->rx_status;
      i = 0;
      dev->packet.pos = 0;
      dev->packet.length = data_rx_len +1;
      netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
      got_data_rx_len = 0;

      xcvr_restart_rx_chain(dev);
    }

    dif->rx_mask = 1;
*/

    DEBUG("[xcvr] DIF handler %0x", dif->rx_status);

    dif->rx_mask &= ~XCVR_DATA_IF_RX_ALMOST_FULL; //disable interrupt
    xcvr->mask &= XCVR_PAYLOAD_READY_MASK;
    xcvr->mask &= XCVR_CRC_VALID_MASK;

    if (dif->rx_status & XCVR_DATA_IF_RX_ALMOST_FULL){

        //dif->rx_mask &= ~XCVR_DATA_IF_RX_ALMOST_FULL; //disable interrupt
        timer_cancel_event(&dev->_internal.rx_timeout_timer);

        xcvr_read_fifo(dev, &dev->packet.buf[dev->packet.pos], dev->packet.fifothresh);
        dev->packet.pos += dev->packet.fifothresh;

        if (dev->packet.length == 0)
        {
                 //DEBUG("[xcvr] packet buf %i", dev->packet.pos);
                 //DEBUG_DATA(dev->packet.buf, dev->packet.pos);

                 dev->packet.length = dev->packet.buf[0] + 1;
                 DEBUG("[xcvr] len %02x", dev->packet.length);

                 // WORKAROUND !!!
                 if (dev->packet.length < dev->packet.fifothresh)
                    dev->packet.pos = dev->packet.length;

                 //DEBUG("[xcvr] RX Packet Length: %i ", dev->packet.length);
        }

        while(!(xcvr_is_rx_fifo_empty()) && (dev->packet.pos < dev->packet.length))
            dev->packet.buf[dev->packet.pos++] = dif->rx_data ;

        uint8_t remaining_bytes = dev->packet.length - dev->packet.pos;
        /*DEBUG("[xcvr] read %i bytes, %i remaining, RX status %x \n", dev->packet.pos,
               remaining_bytes, dif->rx_status);
        */
        if (remaining_bytes == 0) {
            //WORKAROUND
        	dev->packet.crc_status = HW_CRC_UNAVAILABLE;

        	netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);

            xcvr_restart_rx_chain(dev);

            // Restart the reception until upper layer decides to stop it
/*            dev->packet.fifothresh = 0;
            dev->packet.length = 0;
            dev->packet.pos = 0;

            dif->rx_thr = XCVR_DATA_IF_RX_THRESHOLD;
            dev->packet.fifothresh = XCVR_DATA_IF_RX_THRESHOLD;

            xcvr_flush_rx_fifo();
            dif->rx_mask |= XCVR_DATA_IF_RX_ALMOST_FULL;
            xcvr->mask = ((xcvr->mask) & XCVR_PAYLOAD_READY_MASK) | XCVR_PAYLOAD_READY;
            //xcvr->mask = (xcvr->mask & XCVR_CRC_VALID_MASK) | XCVR_CRC_VALID_ON;*/

            // TODO Trigger a manual restart of the Receiver chain (no frequency change) ???
            return;
        }

        //Trigger FifoLevel interrupt
        if (remaining_bytes > XCVR_FIFO_MAX_SIZE)
            dif->rx_mask |= XCVR_DATA_IF_RX_ALMOST_FULL;

        //xcvr->mask = ((xcvr->mask) & XCVR_PAYLOAD_READY_MASK) | XCVR_PAYLOAD_READY;
        xcvr->mask = (xcvr->mask & XCVR_CRC_VALID_MASK) | XCVR_CRC_VALID_ON;
    }
    else
    {
         DEBUG("[xcvr] Unexpected interrupt RX status = %x", dif->rx_status);
    }
}

void _on_data_if_tx_interrupt(void *arg)
{
    /* Get interrupt context */
    ciot25_xcvr_t *dev = (ciot25_xcvr_t *) arg;

    if ((dif->tx_status & XCVR_DATA_IF_TX_FULL) || (dif->tx_status & XCVR_DATA_IF_TX_OVERRUN))
        DEBUG("[xcvr] FULL !!!!! tx_status = %02x", dif->tx_status);
    else if (dif->tx_status & XCVR_DATA_IF_TX_ALMOST_EMPTY){
        fill_in_fifo(dev);
    }
    else
        DEBUG("[xcvr] tx_status = %02x", dif->tx_status);
}
