/*
 * Copyright (C) 2018 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @ingroup     drivers_periph_watchdog
 * @{
 *
 * @file
 * @brief       watchdog driver implementation
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @}
 */


#include "periph_cpu.h"

#ifdef USE_PERIPH_CORTUS_WATCHDOG

void watchdog_enable(unsigned long ulBase)
{
   (void)ulBase;

   wdt->key = 0x700edc33;
   wdt->value = 0xc8000000; // 3200M cycles @ 12.5MHz

   wdt->key = 0x700edc33;
   wdt->status = 1;

   wdt->key = 0x700edc33;
   wdt->restart = 1;

   wdt->key = 0x700edc33;
   wdt->sel_clk = 2; // 12.5MHz

   wdt->key = 0x700edc33;
   wdt->enable = 1;
}



void watchdog_clear(void)
{
   wdt->key = 0x700edc33;
   wdt->restart = 1;
}

#endif /*USE_PERIPH_CORTUS_WATCHDOG*/
