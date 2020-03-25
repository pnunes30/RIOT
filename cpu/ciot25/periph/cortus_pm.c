/*
 * Copyright (C) 2018 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @ingroup     drivers_periph_pm
 * @{
 *
 * @file
 * @brief       Low-level power management driver implementation
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */

#include <stdlib.h>
#include "cpu.h"
#include "periph/pm.h"

#ifdef USE_PERIPH_CORTUS_PM

#if !defined(MODULE_PM_LAYERED) && !defined(PROVIDES_PM_SET_LOWEST_APS)
void pm_set_lowest(void)
{
    aps_sleep(1);
}
#endif

void pm_reboot(void)
{
    abort(); // To confirm (trap #0)
}

void pm_off(void)
{
    abort(); // To confirm
}

#endif /*USE_PERIPH_CORTUS_PM*/
