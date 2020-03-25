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
 * @brief       Implementation of D7A main functions
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */
#include <stdio.h>
#include <string.h>

#include "d7a.h"
#include "d7ap.h"
#include "d7a_netdev.h"
#include "thread.h"
#include "errno.h"

#include "net/netdev.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static void log_print_data(uint8_t* message, uint32_t length)
{
    for( uint32_t i=0 ; i<length ; i++ )
    {
        printf(" %02X", message[i]);
    }
}

#define D7A_CLIENT_NOT_REGISTERED   0xFF
#define D7A_MAX_ADDRESSEE_COUNT     (8)

/**
 * @name    Default configuration for D7A network
 * @{
 */
#ifndef D7A_BROADCAST_ACCESS_CLASS
#define D7A_BROADCAST_ACCESS_CLASS     0x01
#endif

#define DEFAULT_RESP_LEN           (32)  //???

static char d7a_thread_stack[2 * THREAD_STACKSIZE_MAIN];
static uint8_t d7a_instance = D7A_CLIENT_NOT_REGISTERED;

typedef struct
{
    d7a_receive_callback_t callback;
    void *arg;
} user_callback_t;

user_callback_t user_cb;

d7ap_addressee_t broadcast_addr = {
    .ctrl = {
        .nls_method = AES_NONE,
        .id_type = ID_TYPE_NOID,
    },
    .access_class = D7A_BROADCAST_ACCESS_CLASS,
    .id = 0
};

static d7ap_addressee_t current_addressee;

static d7ap_addressee_t known_addressee[D7A_MAX_ADDRESSEE_COUNT];
static uint8_t addressee_count = 0;

static d7ap_session_config_t session_config = {
  .qos = {
    .qos_resp_mode = SESSION_RESP_MODE_ANY,
    .qos_retry_mode = SESSION_RETRY_MODE_NO
  },
  .dormant_timeout = 0,
  .addressee = {
    .ctrl = {
      .nls_method = AES_NONE,
      .id_type = ID_TYPE_NOID,
    },
    .access_class = D7A_BROADCAST_ACCESS_CLASS,
    .id = 0
  }
};

uint8_t get_access_class_from_address(d7a_address *addr)
{
    for(int i = 0; i < addressee_count; i++)
    {
        if (memcmp(known_addressee[i].id, addr, sizeof(d7a_address)) == 0)
            return known_addressee[i].access_class;
    }

    return 0;
}

int register_remote_addressee(d7ap_addressee_t *remote)
{
    if (addressee_count >= D7A_MAX_ADDRESSEE_COUNT)
        return -1;

    known_addressee[addressee_count] = *remote;
    addressee_count++;
    return 0;
}


static void response_callback(uint16_t trans_id, uint8_t* payload,uint8_t len, d7ap_session_result_t result)
{
    (void)trans_id; //parameter not used

    current_addressee = result.addressee;
    if (!addressee_count || (!get_access_class_from_address((d7a_address *)result.addressee.id)))
    {
        register_remote_addressee(&result.addressee);
    }

    DEBUG("Forward response to the client\n");
    log_print_data(payload, len);


    if (user_cb.callback)
        user_cb.callback(user_cb.arg, payload, len, (d7a_address *)result.addressee.id);
}

static bool command_callback(uint8_t* payload, uint8_t len, d7ap_session_result_t result)
{
    current_addressee = result.addressee;

    DEBUG("Forward unsolicited request to the client\n");

    if (user_cb.callback)
        user_cb.callback(user_cb.arg, payload, len, (d7a_address *)result.addressee.id);

    // Assume for now, that no response is expected
    return false;
}

static void command_completed_callback(uint16_t trans_id, error_t error)
{
    (void)trans_id;

    if (error)
    {
        DEBUG("The request has been transmitted with error %d\n", error);
    }
    else
    {
        DEBUG("The request has been transmitted successfully\n");
    }
}

void d7a_init(void)
{
    /* init random */
    //FIXME init random for the D7A stack?

    /* setup netdev modules */
    netdev_t *netdev = (netdev_t*) &xcvr_ressource;

    // Create the thread for the D7A MAC layer
    d7a_netdev_init(d7a_thread_stack, sizeof(d7a_thread_stack), THREAD_PRIORITY_MAIN - 5, "d7a", netdev);
}

void d7a_register_receive_callback(d7a_receive_callback_t callback, void *arg)
{
    d7ap_resource_desc_t desc = {
        .receive_cb = response_callback,
        .transmitted_cb = command_completed_callback,
        .unsolicited_cb = command_callback
    };

    d7a_instance = d7ap_register(&desc);
    user_cb.callback = callback;
    user_cb.arg = arg;

    DEBUG("Client registered with instance %d\n", d7a_instance);
}

void d7a_unregister(void)
{
    //TODO implement d7ap_unregister
    /*if (d7a_instance != D7A_CLIENT_NOT_REGISTERED)
        d7ap_unregister(d7a_instance);*/
}

int d7a_unicast(const uint8_t* payload, uint8_t len, d7a_address* sendto)
{
    uint16_t trans_id = 0;
    error_t res;
    uint8_t expected_response_len = DEFAULT_RESP_LEN;

    DEBUG("D7A unicast\n");
    log_print_data(payload, len);

    //sanity checks
    if (d7a_instance == D7A_CLIENT_NOT_REGISTERED)
    {
        DEBUG("client need to register first\n");
        return -EPERM;
    }

    if (sendto == NULL)
    {
        DEBUG("Destination address is missing\n");
        return -EPERM;
    }

    if (memcmp(current_addressee.id, sendto, sizeof(d7a_address)) != 0)
    {
        memcpy(session_config.addressee.id, sendto->address64, sizeof(session_config.addressee.id));
        // lookup access_class
        session_config.addressee.access_class = get_access_class_from_address(sendto);
    }
    else
        session_config.addressee = current_addressee;

    if ((session_config.qos.qos_resp_mode == SESSION_RESP_MODE_NO) || (session_config.qos.qos_resp_mode == SESSION_RESP_MODE_NO_RPT))
        expected_response_len = 0;

    res = d7ap_send(d7a_instance, &session_config, (uint8_t *)payload, len, expected_response_len, &trans_id);
    if ( res != 0)
    {
        DEBUG("d7ap_send failed\n");
        return 1;
    }

    return res;
}

int d7a_broadcast(const uint8_t* payload, uint8_t len)
{
    uint16_t trans_id = 0;
    error_t res;
    uint8_t expected_response_len = DEFAULT_RESP_LEN;

    DEBUG("D7A broadcast\n");

    //sanity checks
    if (d7a_instance == D7A_CLIENT_NOT_REGISTERED)
    {
        DEBUG("client need to register first\n");
        return -EPERM;
    }

    session_config.addressee = broadcast_addr;
    if ((session_config.qos.qos_resp_mode == SESSION_RESP_MODE_NO) || (session_config.qos.qos_resp_mode == SESSION_RESP_MODE_NO_RPT))
            expected_response_len = 0;

    res = d7ap_send(d7a_instance, &session_config, (uint8_t *)payload, len, expected_response_len, &trans_id);
    if ( res != 0)
    {
        DEBUG("d7ap_send failed\n");
        return 1;
    }

    return res;
}

void d7a_get_node_address(d7a_address* addr)
{
    d7ap_addressee_t devaddr;

    devaddr.ctrl.id_type = d7ap_get_dev_addr(devaddr.id);

    uint8_t len = d7ap_addressee_id_length(devaddr.ctrl.id_type);

    memset(addr->address64, 0, sizeof(addr->address64));
    memcpy(addr->address64, devaddr.id, len);
}
