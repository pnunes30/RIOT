/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     pkg_d7a
 * @file
 * @brief       Implementation of D7A random platform abstraction
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */

#include <stdint.h>
#include <stdlib.h>

#include "framework/inc/random.h"
//#include "random.h"
#include "types.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

//TODO wrapper against the TRNG


__LINK_C uint32_t get_rnd(void)
{
    return (uint32_t) rand();
}

__LINK_C void set_rng_seed(unsigned int seed)
{
    srand(seed);
}
