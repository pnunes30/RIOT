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
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @}
 */

#include <stdint.h>
#include "irq.h"
#include "cpu.h"
#include "machine/cpu.h"


volatile uint8_t __in_isr = 0;

/**
 * @brief Disable all maskable interrupts
 */
unsigned int irq_disable(void)
{
    cpu_int_disable();
    return 0; // do we need to return the state ?
}

/**
 * @brief Enable all maskable interrupts
 */
__attribute__((used)) unsigned int irq_enable(void)
{
    cpu_int_enable();
    return 0;
}

/**
 * @brief Restore the state of the IRQ flags
 */
void irq_restore(unsigned int state)
{
    if (state)
        cpu_int_enable();
}

/**
 * @brief See if the current context is inside an ISR
 */
int irq_is_in(void)
{
    return __in_isr;
}
