/*
 * Copyright (C) 2018 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @ingroup     drivers_periph_timer
 * @{
 *
 * @file
 * @brief       Low-level timer driver implementation
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @}
 */


#include "cpu.h"
#include "periph/timer.h"
#include "periph_conf.h"
#include "periph_cpu.h"

#include "machine/timer_cmp.h"
#include "machine/timer_cap.h"
#include "machine/ic.h"
/**
 * @brief   Interrupt context for each configured timer
 */
static timer_isr_ctx_t isr_ctx[TIMER_NUMOF];

/**
 * @brief   Get the timer device
 */
static inline Timer *dev(tim_t tim)
{
    return timer_config[tim].dev;
}

/**
 * @brief   Get the compare block for timer device
 */
static inline Timer_Cmp *cmpb(tim_t tim, int channel)
{
    return timer_config[tim].cmpb[channel];
}

int timer_init(tim_t tim, unsigned long freq, timer_cb_t cb, void *arg)
{
    /* check if device is valid */
    if (tim >= TIMER_NUMOF) {
        return -1;
    }

    /* remember the interrupt context */
    isr_ctx[tim].cb = cb;
    isr_ctx[tim].arg = arg;

    /* Select the peripheral clock */
    dev(tim)->enable = 0;
    dev(tim)->tclk_sel = timer_config[tim].clk;

    Timer_Cmp *cmpb;
    for( int channel = 0; channel < TIMER_CHAN; channel++)
    {
        cmpb = timer_config[tim].cmpb[channel];
        cmpb->status = 0;
        cmpb->enable = 0;
    }

    Timer_Cap *capb = timer_config[tim].capb;
    capb->status = 0;
    capb->enable = 0;
    capb->in_cap_sel = 0x4; // software capture

    /* configure the timer as upcounter in continuous mode */
    dev(tim)->period  = timer_config[tim].max;

    /* set prescaler */
    dev(tim)->prescaler = ((clock_source(timer_config[tim].clk) / freq) - 1); // --> to confirm with digital team

    /* enable the timer's interrupt */
    NVIC_EnableIRQ(timer_config[tim].irqn);
    /* reset the counter and start the timer */
    timer_start(tim);

    return 0;
}

int timer_set_absolute(tim_t tim, int channel, unsigned int value)
{
    Timer_Cmp *cmpb;

    if (channel >= (int)TIMER_CHAN) {
        return -1;
    }

    cmpb = timer_config[tim].cmpb[channel];
    //start_atomic();
    cmpb->status = 0;
    cmpb->enable = 0;

    cmpb->compare = (value & timer_config[tim].max);
    uint8_t irqn = timer_config[tim].irqn[channel];
    irq[irqn].ipl = 0;
    irq[irqn].ien = 1;

    cmpb->enable = 1;
    cmpb->mask = 1;
    //end_atomic();

    return 0;
}

int timer_clear(tim_t tim, int channel)
{
    Timer_Cmp *cmpb;

    if (channel >= (int)TIMER_CHAN) {
        return -1;
    }

    cmpb = timer_config[tim].cmpb[channel];
    //start_atomic();
    cmpb->status = 0;
    cmpb->enable = 0;
    cmpb->mask = 0;
    //end_atomic();
    return 0;
}

unsigned int timer_read(tim_t tim)
{
    Timer_Cap *capb = timer_config[tim].capb;

    capb->capture = 1;
    while(!capb->status);

    uint32_t value = capb->value;

    capb->capture = 0;
    capb->status = 0;

    return value;
}

void timer_start(tim_t tim)
{
    dev(tim)->enable = 1;
}

void timer_stop(tim_t tim)
{
    dev(tim)->enable = 0;
}

static inline void irq_handler(tim_t tim, int channel)
{
    uint32_t status = timer_config[tim].cmpb[channel]->status;

    if (status && isr_ctx[tim].cb) {
        isr_ctx[tim].cb(isr_ctx[tim].arg, channel);
    }

    aps_isr_end();
}

void interrupt_handler(IRQ_TIMER1_CMPA)
{
    __enter_isr();
    irq_handler(0, 0);
    __exit_isr();
}

void interrupt_handler(IRQ_TIMER1_CMPB)
{
    __enter_isr();
    irq_handler(0, 1);
    __exit_isr();
}

void interrupt_handler(IRQ_TIMER2_CMPA)
{
    __enter_isr();
    irq_handler(1, 0);
    __exit_isr();
}

void interrupt_handler(IRQ_TIMER2_CMPB)
{
    __enter_isr();
    irq_handler(1, 1);
    __exit_isr();
}
