/*
 * Copyright (C) 2014-2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    cpu_aps
 * @ingroup     cpu
 * @brief       Common implementations and headers for APS family based
 *              micro-controllers
 * @{
 *
 * @file
 * @brief       Basic definitions for the APS CPU
 *
 * When ever you want to do something hardware related, that is accessing MCUs
 * registers, just include this file. It will then make sure that the MCU
 * specific headers are included.
 *
 * @author      Stefan Pfeiffer <stefan.pfeiffer@fu-berlin.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Joakim Nohlgård <joakim.nohlgard@eistec.se>
 *
 * @todo        remove include irq.h once core was adjusted
 */

#ifndef CPU_H
#define CPU_H

#include <stdio.h>

#include "irq.h"
#include "sched.h"
#include "thread.h"
#include "cpu_conf.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief global in-ISR state variable
 */
extern volatile uint8_t __in_isr;

/**
 * @brief Flag entering of an ISR
 */
static inline void __enter_isr(void)
{
    __in_isr = 1;
}

/**
 * @brief Flag exiting of an ISR
 */
static inline void __exit_isr(void)
{
    __in_isr = 0;
}

/**
 * @brief   Initialization of the CPU
 */
void cpu_init(void);

/**
 * @brief   Prints the current content of the link register (lr)
 */
static inline void cpu_print_last_instruction(void)
{
    //uint32_t *lr_ptr;
    //__asm__ __volatile__("mov %0, lr" : "=r"(lr_ptr));
    //printf("%p\n", (void*) lr_ptr);
}

/**
 * @brief   Put the CPU into (deep) sleep mode, using the `WFI` instruction
 *
 * @param[in] deep      !=0 for deep sleep, 0 for light sleep
 */
static inline void aps_sleep(int deep)
{
    if (deep) {
        //SCB->SCR |=  (SCB_SCR_SLEEPDEEP_Msk);
    }
    else {
        //SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk);
    }

    /* ensure that all memory accesses have completed and trigger sleeping */
    unsigned state = irq_disable();
    //__DSB();
    //__WFI();
    irq_restore(state);
}

/**
 * @brief   Trigger a conditional context scheduler run / context switch
 *
 * This function is supposed to be called in the end of each ISR.
 */
static inline void aps_isr_end(void)
{
    if (sched_context_switch_request) {
        thread_yield();
    }
}

#ifdef __cplusplus
}
#endif

#endif /* CPU_H */
/** @} */
