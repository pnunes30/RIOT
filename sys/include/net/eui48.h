/*
 * Copyright (C) 2018 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_eui48 IEEE EUI-48 identifier
 * @ingroup     net
 * @brief       Definition and IPv6 IID conversion for IEEE EUI-48 identifiers
 * @{
 *
 * @file
 * @brief       Definition and IPv6 IID conversion for IEEE EUI-48 identifiers
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef NET_EUI48_H
#define NET_EUI48_H

#include <stdint.h>

#include "net/eui64.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Data type to represent an EUI-48
 */
typedef union {
    uint8_t uint8[6];            /**< split into 6 8-bit words. */
    network_uint16_t uint16[3];  /**< split into 3 16-bit words. */
} eui48_t;

/**
 * @brief   Generates an IPv6 interface identifier from a 48-bit device address
 *
 * @see     [RFC 2464, section 4](https://tools.ietf.org/html/rfc2464#section-4)
 * @see     [RFC 4291, section 2.5.1](https://tools.ietf.org/html/rfc4291#section-2.5.1)
 *
 * @param[out] iid      the resulting EUI-64.
 * @param[in]  addr     a 48-bit device address
 */
static inline void eui48_to_ipv6_iid(eui64_t *iid, const eui48_t *addr)
{
    iid->uint8[0] = addr->uint8[0] ^ 0x02;
    iid->uint8[1] = addr->uint8[1];
    iid->uint8[2] = addr->uint8[2];
    iid->uint8[3] = 0xff;
    iid->uint8[4] = 0xfe;
    iid->uint8[5] = addr->uint8[3];
    iid->uint8[6] = addr->uint8[4];
    iid->uint8[7] = addr->uint8[5];
}

/**
 * @brief   Convert a 64-bit IPv6 IID into a EUI-48 device address
 *
 * @param[out] addr     the resulting EUI-48
 * @param[in]  iid      a 64-bit IPv6 interface identifier
 */
static inline void eui48_from_ipv6_iid(eui48_t *addr, const eui64_t *iid)
{
    addr->uint8[0] = iid->uint8[0] ^ 0x02;
    addr->uint8[1] = iid->uint8[1];
    addr->uint8[2] = iid->uint8[2];
    addr->uint8[3] = iid->uint8[5];
    addr->uint8[4] = iid->uint8[6];
    addr->uint8[5] = iid->uint8[7];
}

#ifdef __cplusplus
}
#endif

#endif /* NET_EUI48_H */
/** @} */
