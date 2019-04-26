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
 * @brief       Implementation of the kernels irq interface
 *
 * @brief       IEN manipulation with unclobbered CC and unclobbered PIEN.
 *
 * - PIEN is a HW flag toggled by trap/rti. It should never be
 *   changed/manipulated directly!!!
 * - IEN can be toggled at will, but without clobbering CC, as
 *   the compiler may not always understand 'psr clobber'.
 * - irq_restore must be used in pair with a temporary irq_disable
 *   (or enable?).
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @}
 */
// FIXME: APS BSP buggy (cpu_int_disable/enable in machine/cpu.h).
// FIXME: RIOT refuses to make them inline

#include <stdint.h>
#include "irq.h"
#include "cpu.h"
#include "periph_cpu.h"


volatile uint8_t __in_isr = 0;

/* Notes:
 * Preserving PIEN and CC is mandatory: because PIEN has a
 * hw meaning only, while CC allows compilation optimizations
 * (even if psr is marked clobbered for sanity).
 * PIEN=1 is always true after rti, context_restore and in
 * pure HW ISR context, but this might be false in non
 * maskable interrupt or sw trap
 * => the Bsp cpu_int_enable() or cpu_int_disable()
 *    are not applicable in an OS context.
 *
 * Regarding the use of extended asm, the optimization trick
 * here is that the compiler will select r2 and r7 without
 * wasting stack pop/push or any additional registers.
 *
 * However, it is not as efficient as inlined, but this is
 * a RIOT restriction.
 */


/**
 * @brief Disable all maskable interrupts
 */
unsigned int irq_disable(void)
{
	unsigned int flags, new_flags;
	asm volatile ( "mov    %0, psr\n"
               	   "movhi  %1, #0xfffe\n"
                   "add    %1, #0xffff\n"
                   "and    %1, %0\n"
                   "mov    psr,  %1\n"
                   : /* output */ "=r" (flags), "=r" (new_flags)
                   : /* no input */
                   : /* clobber */ "psr");
    return flags;
}

/**
 * @brief Enable all maskable interrupts
 */
__attribute__((used)) unsigned int irq_enable(void)
{
	unsigned int flags, new_flags;
    asm volatile ( "mov    %0, psr\n"
                   "movhi  %1, #0x1\n"
                   "or     %1, %0\n"
                   "mov    psr,  %1\n"
                   : /* output */ "=r" (flags), "=r" (new_flags)
                   : /* no input */
                   : /* clobber */ "psr" );
    return flags;
}

/**
 * @brief Restore the state of the IRQ flags
 */
void irq_restore(unsigned int flags)
{
    unsigned int old_flags=0;
    flags &= (0xffff0000);

    asm volatile ("mov    %1, psr\n"
                  "and    %1,#0xffff\n"
                  "add    %1, %0\n"
                  "mov    psr, %1\n"
                  : /* output */ "+r" (flags), "=r" (old_flags)
                  : /* input */
                  : /* clobber*/ "psr" );
}

/**
 * @brief returns the IEN==1 status
 */
int irq_enabled(void)
{
	unsigned int flags;
    asm volatile ( "mov    %0, psr\n"
                   : /* output */ "=r" (flags)
                   : /* no input */
                   : /* no clobber */);
    return ((flags & 0x00010000) == 0x00010000);
}

/**
 * @brief See if the current context is inside an ISR
 */
inline int irq_is_in(void)
{
    return __in_isr;
}
