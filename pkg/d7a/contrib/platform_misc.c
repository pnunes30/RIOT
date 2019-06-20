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
 * @brief       Implementation of D7A platform abstraction
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */

#include "framework/hal/inc/hwwatchdog.h"
#include "framework/hal/inc/hwsystem.h"
#include "framework/hal/inc/hwatomic.h"
#include "framework/inc/link_c.h"
#include "d7a.h"

#include "irq.h"
#include "xtimer.h"

static unsigned int cpsr;
static unsigned int in_atomic=0;    //for nested atomic sections

//#warning "Non thread safe start_atomic"
//On multithread (riot), each thread has its own psr on context switch
//hence those functions and variables must not be scalars but per thread.
//The easiest workaround is to drop the global cpsr and use
//local cpsr variables with local macros (cf aps_atomic_start in cpu.h)

void hw_busy_wait(int16_t microseconds)
{
    xtimer_usleep(microseconds);
}

void hw_watchdog_feed(void)
{
   //TODO wrapper to the RIOT watchdog APIs
   // for now, watchdog is not initialized
}

__LINK_C uint64_t hw_get_unique_id(void)
{
    return 0x1122334455667788;
}

__LINK_C void start_atomic(void)
{
    if (in_atomic++==0) {          //no need for atomic_inc on non smp, non preemptive systems.
        cpsr = irq_disable();
    }

    //assert( (!irq_enabled()) );   //multithread case // Do we really need this code?
}

__LINK_C void end_atomic(void)
{
    assert(in_atomic>0);
    if (--in_atomic==0) {
        irq_restore(cpsr);
    }
}

#ifdef APS_MINICLIB
//function declared in <framework/inc/debug.h>
//OSS7 overrided __assert_func to abort the CPU and get the stack trace
/*NORETURN*/ void __assert_func(const char *file, int line, const char *func, const char *failedexpr)
{
    (void)func;
    (void)failedexpr;

    assert_break();
#ifdef DEBUG_ASSERT_VERBOSE
    _assert_failure(file, line);
#else
#ifndef DEVELHELP
    /* DEVELHELP not set => reboot system */
    pm_reboot();
#else
    /* DEVELHELP set => power off system */
    pm_off();
#endif
#endif
}
#endif /*APS_MINICLIB*/

