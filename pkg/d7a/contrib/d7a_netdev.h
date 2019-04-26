/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net
 *
 * @{
 * @file
 * @author  Philippe Nunes <philippe.nunes@cortus.com>
 */

#ifndef D7A_NETDEV_H
#define D7A_NETDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "net/netopt.h"
#include "net/netdev.h"
#include "thread.h"

// FIXME How to choose a value for the MSG_TYPE_EVENT ?
#define D7A_TIMEOUT_MSG_TYPE_EVENT (0x2235)       /**< xtimer message receiver event*/
#define D7A_NETDEV_MSG_TYPE_EVENT  (0x2236)       /**< message received from driver */
#define D7A_MSG_TYPE_RECV          (0x2238)       /**< event for frame reception */


/**
 * @brief   Starts D7A thread.
 *
 * @param[in]  stack              pointer to the stack designed for D7A
 * @param[in]  stacksize          size of the stack
 * @param[in]  priority           priority of the D7A stack
 * @param[in]  name               name of the D7A stack
 * @param[in]  netdev             pointer to the netdev interface
 *
 * @return  PID of D7A thread
 * @return  -EINVAL if there was an error creating the thread
 */
int d7a_netdev_init(char *stack, int stacksize, char priority, const char *name, netdev_t *netdev);


#ifdef __cplusplus
}
#endif

#endif /* D7A_NETDEV_H */
/** @} */
