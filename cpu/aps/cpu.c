/*
 * Copyright (C) 2018 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @{
 *
 * @file
 * @brief       Implementation of the CPU initialization
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @}
 */

#include <stdint.h>
#include "cpu.h"
#include "irq.h"

#include "periph_conf.h"
#include "periph/init.h"


static void cpu_clock_init(void)
{
    //TODO
}

/**
 * @brief   Initialize the CPU, set IRQ priorities
 */
void cpu_init(void)
{
    /* initialize the APS core */
    //aps_init();
    /* initialize the clock system */
    cpu_clock_init();
    /* trigger static peripheral initialization */
    periph_init();
}
