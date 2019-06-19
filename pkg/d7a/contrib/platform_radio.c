/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net
 * @file
 * @brief       Implementation of D7A radio platform abstraction
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "string.h"
#include "framework/inc/types.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#include "d7a_netdev.h"
#include "net/netdev.h"
#include "net/netopt.h"
#include "framework/hal/inc/hwradio.h"
#include "framework/inc/errors.h"

#if (ENABLE_DEBUG)
static void log_print_data(uint8_t* message, uint32_t length);
#define DPRINT(...) DEBUG_PRINT(__VA_ARGS__)
#define DPRINT_DATA(...) log_print_data(__VA_ARGS__)
#else
#define DPRINT(...)
#define DPRINT_DATA(...)
#endif

static alloc_packet_callback_t alloc_packet_callback;
static release_packet_callback_t release_packet_callback;
static rx_packet_callback_t rx_packet_callback;
static tx_packet_callback_t tx_packet_callback;
static rx_packet_header_callback_t rx_packet_header_callback;
static tx_refill_callback_t tx_refill_callback;

static netdev_t *netdev;

#if (ENABLE_DEBUG)
static void log_print_data(uint8_t* message, uint32_t length)
{
    for( uint32_t i=0 ; i<length ; i++ )
    {
        printf(" %02X", message[i]);
    }
}
#endif

hw_radio_state_t hw_radio_get_opmode(void)
{
    hw_radio_state_t opmode;

    netdev->driver->get(netdev, NETOPT_STATE, &opmode, sizeof(netopt_state_t));
    return opmode;
}

void hw_radio_set_opmode(hw_radio_state_t opmode) {
    netdev->driver->set(netdev, NETOPT_STATE, &opmode, sizeof(netopt_state_t));
}

void hw_radio_set_center_freq(uint32_t center_freq)
{
    DPRINT("Set center frequency: %d\n", center_freq);

    netdev->driver->set(netdev, NETOPT_CHANNEL_FREQUENCY, &center_freq, sizeof(uint32_t));
}

void hw_radio_set_rx_bw_hz(uint32_t bw_hz)
{
    netdev->driver->set(netdev, NETOPT_BANDWIDTH, &bw_hz, sizeof(uint32_t));
}

void hw_radio_set_bitrate(uint32_t bps)
{
    netdev->driver->set(netdev, NETOPT_BITRATE, &bps, sizeof(uint32_t));
}

void hw_radio_set_tx_fdev(uint32_t fdev)
{
    netdev->driver->set(netdev, NETOPT_FDEV, &fdev, sizeof(uint32_t));
}

void hw_radio_set_preamble_size(uint16_t size)
{
    netdev->driver->set(netdev, NETOPT_PREAMBLE_LENGTH, &size, sizeof(uint16_t));
}

/* TODO Make use of the following APIs to setup the xcvr */
/*
void hw_radio_set_modulation_shaping(uint8_t shaping)
{
    netdev->driver->set(netdev, NETOPT_MOD_SHAPING, &shaping, sizeof(uint8_t));
}

void hw_radio_set_preamble_polarity(uint8_t polarity)
{
    netdev->driver->set(netdev, NETOPT_PREAMBLE_POLARITY, &polarity, sizeof(uint8_t));
}

void hw_radio_set_rssi_threshold(uint8_t rssi_thr)
{
    netdev->driver->set(netdev, NETOPT_CCA_THRESHOLD, &rssi_thr, sizeof(uint8_t));
}

void hw_radio_set_sync_word_size(uint8_t sync_size)
{
    netdev->driver->set(netdev, NETOPT_SYNC_LENGTH, &sync_size, sizeof(uint8_t));
}

void hw_radio_set_sync_on(uint8_t enable)
{
    netdev->driver->set(netdev, NETOPT_SYNC_ON, &enable, sizeof(uint8_t));
}

void hw_radio_set_preamble_detect_on(uint8_t enable)
{
    netdev->driver->set(netdev, NETOPT_PREAMBLE_DETECT_ON, &enable, sizeof(uint8_t));
}
*/

void hw_radio_set_rssi_smoothing(uint8_t rssi_samples)
{
    netdev->driver->set(netdev, NETOPT_RSSI_SMOOTHING, &rssi_samples, sizeof(uint8_t));
}

void hw_radio_set_dc_free(uint8_t scheme)
{
    netdev->driver->set(netdev, NETOPT_DC_FREE_SCHEME, &scheme, sizeof(uint8_t));
}

void hw_radio_set_sync_word(uint8_t *sync_word, uint8_t sync_size)
{
    //uint8_t sync_size = read_reg(REG_SYNCCONFIG) & ~RF_SYNCCONFIG_SYNCSIZE_MASK;
    assert(sync_size >= 1);

    netdev->driver->set(netdev, NETOPT_SYNC_WORD, sync_word, sync_size);
}

void hw_radio_set_crc_on(uint8_t enable)
{
    netdev->driver->set(netdev, NETOPT_INTEGRITY_CHECK, &enable, sizeof(uint8_t));
}

int16_t hw_radio_get_rssi(void)
{
    int16_t rssi;

    netdev->driver->get(netdev, NETOPT_RSSI_VALUE, (uint16_t *)&rssi, sizeof(uint16_t));
    return rssi;
}

void hw_radio_set_payload_length(uint16_t length)
{
    netdev->driver->set(netdev, NETOPT_MAX_PACKET_SIZE, &length, sizeof(uint16_t));
}

error_t hw_radio_send_payload(uint8_t * data, uint16_t len)
{
    error_t ret;
    iolist_t iolist = {
            .iol_base = data,
            .iol_len = len
        };

    DPRINT("TX data");
    DPRINT("Payload: %d bytes", len);
    DPRINT_DATA(data, len);

    ret = netdev->driver->send(netdev, &iolist);
    if ( ret == -ENOTSUP)
    {
        DPRINT("Cannot send: radio is still transmitting");
    }

    return ret;
}

error_t hw_radio_set_idle(void)
{
    // TODO Select the chip mode during Idle state (Standby mode or Sleep mode)

    // For now, select by default the standby mode
    hw_radio_set_opmode(HW_STATE_SLEEP);

    return SUCCESS;
}

bool hw_radio_is_idle(void)
{
    if (hw_radio_get_opmode() == HW_STATE_STANDBY)
        return true;
    else
        return false;
}

static void _event_cb(netdev_t *dev, netdev_event_t event)
{
    int len = 0;
    netdev_radio_rx_info_t packet_info;

    switch (event) {
        case NETDEV_EVENT_ISR:
            {
                msg_t msg;
                kernel_pid_t _pid = d7a_get_pid();
                assert(_pid != KERNEL_PID_UNDEF);

                msg.type = D7A_NETDEV_MSG_TYPE_EVENT;
                msg.content.ptr = dev;

                if (msg_send(&msg, _pid) <= 0) {
                    DPRINT("d7a_netdev: possibly lost interrupt.\n");
                }
                break;
            }

        case NETDEV_EVENT_RX_COMPLETE:
            len = dev->driver->recv(netdev, NULL, 0, 0);
            hw_radio_packet_t* rx_packet = alloc_packet_callback(len);
            rx_packet->length = len;

            dev->driver->recv(netdev, rx_packet->data, len, &packet_info);
            DPRINT("RX done\n");
            DPRINT("Payload: %d bytes, RSSI: %i, LQI: %i" /*SNR: %i, TOA: %i}\n"*/,
                    len, packet_info.rssi, packet_info.lqi/*(int)packet_info.snr,
                    (int)packet_info.time_on_air*/);
            for( int i=0; i < len; i++ )
            {
                printf(" %02X", rx_packet->data[i]);
            }
            //DPRINT_DATA(rx_packet->data, len);

            rx_packet->rx_meta.timestamp = timer_get_counter_value();
            rx_packet->rx_meta.crc_status = HW_CRC_UNAVAILABLE;
            rx_packet->rx_meta.rssi = packet_info.rssi;
            rx_packet->rx_meta.lqi = packet_info.lqi;

            rx_packet_callback(rx_packet);
            break;
        case NETDEV_EVENT_TX_COMPLETE:
            DPRINT("Transmission completed");
            if(tx_packet_callback) {
                tx_packet_callback(timer_get_counter_value());
            }
            break;
        case NETDEV_EVENT_TX_REFILL_NEEDED:
            DPRINT("New data needed to transmit without discontinuity");

            if (tx_refill_callback) {
                uint8_t thr;
                netdev->driver->get(netdev, NETOPT_FIFOTHRESHOLD, &thr, sizeof(uint8_t));
                tx_refill_callback(thr);
            }
            break;
        case NETDEV_EVENT_RX_STARTED:
        {
            uint8_t buffer[4];
            uint8_t len;

            len = dev->driver->recv(netdev, buffer, sizeof(buffer), NULL);
            DPRINT("Packet Header received %i\n", len);
            DPRINT_DATA(buffer, len);

            if(rx_packet_header_callback) {
                rx_packet_header_callback(buffer, len);
            }
        break;
        }
        case NETDEV_EVENT_TX_TIMEOUT:
        case NETDEV_EVENT_RX_TIMEOUT:
            hw_radio_set_idle();
            // TODO invoke a registered callback to notify the upper layer about this event
            break;
        default:
            DPRINT("Unexpected netdev event received: %d\n", event);
            break;
    }
}

error_t hw_radio_init(hwradio_init_args_t* init_args)
{
    error_t ret;

    alloc_packet_callback = init_args->alloc_packet_cb;
    release_packet_callback = init_args->release_packet_cb;
    rx_packet_callback =  init_args->rx_packet_cb;
    rx_packet_header_callback = init_args->rx_packet_header_cb;
    tx_packet_callback = init_args->tx_packet_cb;
    tx_refill_callback = init_args->tx_refill_cb;

    netdev = (netdev_t*) &xcvr; // xcvr should be declared in RADIO_CHIP_netdev.c
    ret = netdev->driver->init(netdev); // done also in gnrc_netif

    netdev->event_callback = _event_cb;

    // put the chip into sleep mode after init
    hw_radio_set_idle();

    return ret;
}

bool hw_radio_is_rx(void)
{
    if (hw_radio_get_opmode() == HW_STATE_RX)
        return true;
    else
        return false;
}

void hw_radio_enable_refill(bool enable)
{
    netopt_enable_t netopt_enable = enable ? NETOPT_ENABLE : NETOPT_DISABLE;
    netdev->driver->set(netdev, NETOPT_TX_REFILL_IRQ, &netopt_enable, sizeof(netopt_enable_t));
}

void hw_radio_enable_preloading(bool enable)
{
    netopt_enable_t netopt_enable = enable ? NETOPT_ENABLE : NETOPT_DISABLE;
    netdev->driver->set(netdev, NETOPT_PRELOADING, &netopt_enable, sizeof(netopt_enable_t));
}

void hw_radio_set_tx_power(int8_t eirp)
{
    netdev->driver->set(netdev, NETOPT_TX_POWER, &eirp, sizeof(int8_t));
}

void hw_radio_set_rx_timeout(uint32_t timeout)
{
    netdev->driver->set(netdev, NETOPT_RX_TIMEOUT, &timeout, sizeof(uint32_t));
}

void hw_radio_enable_rx_interrupt(bool enable_rx_start_irq, bool enable_rx_end_irq)
{
    netopt_enable_t netopt_enable = enable_rx_start_irq ? NETOPT_ENABLE : NETOPT_DISABLE;
    netdev->driver->set(netdev, NETOPT_RX_START_IRQ, &netopt_enable, sizeof(netopt_enable_t));
    netopt_enable = enable_rx_end_irq ? NETOPT_ENABLE : NETOPT_DISABLE;
    netdev->driver->set(netdev, NETOPT_RX_END_IRQ, &netopt_enable, sizeof(netopt_enable_t));
}

void hw_radio_stop(void) {
    // TODO reset chip?

    hw_radio_set_opmode(HW_STATE_SLEEP);
}
