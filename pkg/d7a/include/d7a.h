/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    D7A DASH7 Alliance protocol
 * @ingroup     pkg_d7a
 * @brief       An open source implementation of the DASH7 Alliance protocol
 * @see         https://github.com/MOSAIC-LoPoW/dash7-ap-open-source-stack
 *
 * @{
 *
 * @file
 *
 * @author  Philippe Nunes <philippe.nunes@cortus.com>
 */

#ifndef D7A_H
#define D7A_H

#ifdef __cplusplus
extern "C" {
#endif

#include "thread.h"

/**
 * @brief Used to define the destination or the origin address
 */
typedef struct
{
    uint8_t address64[8];
} d7a_address;


/**
 * @brief   callback used for communicating received packets to the D7A client
 *
 * @param[in] arg       The pointer to the client parameter
 * @param[in] pkt       The pointer to the received packet
 * @param[in] pkt_len   The length of the received packet
 * @param[in] remote    The pointer to the address of the remote end point (transmitter of the received packet)
 */
typedef void (*d7a_receive_callback_t)(void *arg, uint8_t *pkt, uint8_t pkt_len, d7a_address *remote);


/**
 * @brief   Bootstrap D7A MAC layer
 */
void d7a_init(void);

/**
 * @brief   get PID of D7A thread.
 *
 * @return  PID of D7A thread
 */
kernel_pid_t d7a_get_pid(void);

/**
 * @brief   Register the client callback
 *
 * @param[in] callback    The pointer to the client callback for packet reception
 * @param[in] arg         The pointer to the client parameter to pass in the callback argument
 */
void d7a_register_receive_callback(d7a_receive_callback_t callback, void* arg);

/**
 * @brief   Unregister the client callback
 *
 */
void d7a_unregister(void);

/**
 * @brief   Send a unicast packet over D7A.
 *
 * @param[in] payload   The pointer to the payload buffer
 * @param[in] len       The length of the payload
 * @param[in] sendto    The destination address
 * @return 0 on success
 * @return an error (errno.h) in case of failure
 */
int d7a_unicast(const uint8_t* payload, uint8_t len, d7a_address* sendto);


/**
 * @brief   Send a broadcast packet over D7A.
 *
 * @param[in] payload   The pointer to the payload buffer
 * @param[in] len       The length of the payload
 * @return 0 on success
 * @return an error (errno.h) in case of failure
 */
int d7a_broadcast(const uint8_t* payload, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* D7A_H */
/** @} */
