/*
 * Copyright (C) 2017 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup cortus_fpga
 * @{
 */

/**
 * @file
 * @brief       cortus_fpga board initialization
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 */

#include "board.h"
#include "periph/gpio.h"



void board_init(void)
{
    /* initialize the boards LEDs */
    gpio_init(LED0_PIN, GPIO_OUT);

    /* initialize the CPU */
    cpu_init();

    //__watchdog_init(); // done in periph_init ??
}
