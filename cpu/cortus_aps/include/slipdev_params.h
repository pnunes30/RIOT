/*
 * Copyright (C) 2015 Martine Lenders <mlenders@inf.fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief   slip parameters example, used by auto_init_gnrc_netif
 *
 * @author  Martine Lenders <mlenders@inf.fu-berlin.de>
 */
/*
 * APS: this header will be included by default when no other header is specified
 * just 'USEMODULE += slipdev' to enable a slip interface.
 */
#ifndef SLIPDEV_PARAMS_H
#define SLIPDEV_PARAMS_H

#include "slipdev.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SLIP_UART
#define SLIP_UART 1		/*UART2*/
#endif
#ifndef SLIP_BAUDRATE
#define SLIP_BAUDRATE 115200
#endif

static slipdev_params_t slipdev_params[] = {
    {
        .uart = SLIP_UART,
        .baudrate = SLIP_BAUDRATE,
    },
};

#ifdef __cplusplus
}
#endif
#endif /* SLIPDEV_PARAMS_H */
/** @} */
