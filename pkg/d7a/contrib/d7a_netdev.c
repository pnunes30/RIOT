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
 * @brief       Netdev adoption for d7a
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "msg.h"
#include "kernel_types.h"

#include "net/gnrc/netif.h"

#include "d7a_netdev.h"
#include "d7a/timer.h"
#include "framework/inc/d7ap.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define D7A_QUEUE_LEN      (8)
static msg_t _queue[D7A_QUEUE_LEN];

static kernel_pid_t d7a_pid;

/**
 * @name    Default configuration for D7A network
 * @{
 */
#ifndef D7A_DEVICE_ACCESS_CLASS
#define D7A_DEVICE_ACCESS_CLASS     0x01
#endif
/** @} */

#ifdef D7A_AS_GNRC_MODULE
static void response_callback(uint16_t trans_id, uint8_t* payload,uint8_t len, d7ap_session_result_t result)
{
    //TODO
}

static uint8_t command_callback(uint8_t* payload, uint8_t len, d7ap_session_result_t d7asp_result)
{
    //TODO
    return 0;
}

static void command_completed_callback(uint16_t trans_id, error_t error)
{
    if (error)
        printf("The request has been transmitted with error %d", error);
    else
        puts("The request has been transmitted successfully");
}

static const gnrc_netif_ops_t d7a_ops = {
    .init = _d7a_init,
    .send = _send,
    .recv = _recv,
    .get = gnrc_netif_get_from_netdev,
    .set = gnrc_netif_set_from_netdev,
    .msg_handler = _d7a_msg_handler,
};

static void _d7a_init(gnrc_netif_t *netif)
{
    d7ap_resource_desc_t desc = {
        .receive_cb = response_callback,
        .transmitted_cb = command_completed_callback,
        .unsolicited_cb = command_callback
    };

    d7ap_init();
    sInstance = d7ap_register(&desc);
}

static int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt)
{
    gnrc_pktsnip_t *vector;
    int res = -ENOBUFS;
    size_t n;
    uint16_t trans_id;

    if (pkt->type == GNRC_NETTYPE_NETIF) {
        /* we don't need the netif snip: remove it */
        pkt = gnrc_pktbuf_remove_snip(pkt, pkt);
    }

    // prepare session configuration
    d7ap_session_config_t config;

    /* prepare packet for sending */
    vector = gnrc_pktbuf_get_iovec(pkt, &n);
    if (vector != NULL) {
        /* reassign for later release; vector is prepended to pkt */
        pkt = vector;
        struct iovec *v = (struct iovec *)vector->data;
        netdev_t *dev = netif->dev;

        // Loop for until all the snip packets consumed ?
        res = d7ap_send(sInstance, &config, v->iov_base, v->iov_len, 0, &trans_id);
    }
    /* release old data */
    gnrc_pktbuf_release(pkt);
    return res;
}

static void _d7a_msg_handler(gnrc_netif_t *netif, msg_t *msg)
{
    switch (msg->type) {
        case D7A_TIMEOUT_MSG_TYPE_EVENT:
            /* Tell D7A a time event was received */
            {
                DEBUG("[D7A] MAC timer timeout\n");
                timer_event* event = msg->content.ptr;
                event->cb(event->arg);
                break;
            }
        case D7A_NETDEV_MSG_TYPE_EVENT:
            /* Received an event from driver */
            netdev_t *dev = msg.content.ptr;
            dev->driver->isr(dev);
            break;
        default: {
#if ENABLE_DEBUG
            DEBUG("[D7A MAC]: unknown message type 0x%04x"
                  "(no message handler defined)\n", msg->type);
#endif
            break;
        }
    }

    return NULL;
}
#else
static void *_d7a_event_loop(void *arg) {
    (void)arg;

    msg_init_queue(_queue, D7A_QUEUE_LEN);
    netdev_t *dev;
    msg_t msg;

    /* init d7a */
    d7ap_init(); 

    // the D7A stack takes care to initialize the netdev driver
    // netdev->driver->init(netdev);

    while (1) {
        msg_receive(&msg);
        switch (msg.type) {
            case D7A_TIMEOUT_MSG_TYPE_EVENT:
                /* Tell D7A a time event was received */
                {
                    DEBUG("[D7A] MAC timer timeout\n");
                    timer_event* event = msg.content.ptr;
                    event->cb(event->arg);
                    break;
                }
            case D7A_NETDEV_MSG_TYPE_EVENT:
                /* Received an event from driver */
                dev = msg.content.ptr;
                dev->driver->isr(dev);
                break;
            default: {
#if ENABLE_DEBUG
                DEBUG("[D7A MAC]: unknown message type 0x%04x"
                      "(no message handler defined)\n", msg.type);
#endif
                break;
            }
        }
    }

    return NULL;
}
#endif


/* get D7A thread pid */
kernel_pid_t d7a_get_pid(void) {

    if (d7a_pid <= 0)
        return sched_active_pid;
    else
        return d7a_pid;
}

/* starts D7A thread */
int d7a_netdev_init(char *stack, int stacksize, char priority,
                    const char *name, netdev_t *netdev) {

#ifdef D7A_AS_GNRC_MODULE
    gnrc_netif_t  *netif = gnrc_netif_create(stack, stacksize, priority, name, netdev,
                                             &d7a_ops);

    return netif;
#else
    d7a_pid = thread_create(stack, stacksize,
                            priority, THREAD_CREATE_STACKTEST,
                            _d7a_event_loop, netdev, name);

    if (d7a_pid <= 0) {
        return -EINVAL;
    }

    DEBUG("[D7A] thread created for the D7A MAC layer [%d]\n", d7a_pid);
    return d7a_pid;
#endif
}
