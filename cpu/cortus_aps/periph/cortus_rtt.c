/*
 * Copyright (C) 2018 Cortus
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @ingroup     drivers_periph_rtt
 * @{
 *
 * @file
 * @brief       RTT peripheral driver implementation for APS.
 * 				Basic incomplete implementation based on timer periph reuse.
 *
 * @author      <sw@cortus.com>
 * @}
 */

#include "cpu.h"

#include "periph_conf.h"
#include "periph/timer.h"
#include "periph/rtt.h"

#ifdef USE_PERIPH_CORTUS_RTT

typedef struct {
    rtt_cb_t alarm_cb;              /**< callback called from RTC alarm */
    void *   alarm_arg;             /**< argument passed to the callback */
    uint32_t alarm_val;             /**< value of the compare block. FIXME: saved here to workaround the lack of periph_timer interface... */
    rtt_cb_t overflow_cb;           /**< callback called when RTC overflows */
    void *   overflow_arg;          /**< argument passed to the callback */
} rtt_state_t;

static rtt_state_t rtt_state;

void rtt_isr_cb(void *arg, int channel);

void rtt_init(void)
{
	timer_init(RTT_DEV, RTT_FREQUENCY, rtt_isr_cb, /*arg*/NULL);
}

void rtt_set_overflow_cb(rtt_cb_t cb, void *arg)
{
    rtt_state.overflow_cb = cb;
    rtt_state.overflow_arg = arg;
    /* FIXME: enable overflow isr if previously disabled */
    assert(false); /* not used */
}

void rtt_clear_overflow_cb(void)
{
    rtt_state.overflow_cb = NULL;
    rtt_state.overflow_arg = NULL;
    /* FIXME: disable overflow isr */
    assert(false); /* not used */
}

uint32_t rtt_get_counter(void)
{
    return timer_read(RTT_DEV);
}

void rtt_set_counter(uint32_t counter)
{
	/* FIXME: not supported for counter !=0 */
	assert(counter == 0);
	timer_stop(RTT_DEV);
	timer_start(RTT_DEV);
}

void rtt_set_alarm(uint32_t alarm, rtt_cb_t cb, void *arg)
{
    rtt_state.alarm_cb  = cb;
    rtt_state.alarm_arg = arg;
    rtt_state.alarm_val = alarm;
    timer_set_absolute(RTT_DEV, RTT_CHAN, alarm);
}

uint32_t rtt_get_alarm(void)
{
    return rtt_state.alarm_val; ///*cmp->compare*/;
}

void rtt_clear_alarm(void)
{
    rtt_state.alarm_cb  = NULL;
    rtt_state.alarm_arg = NULL;
    rtt_state.alarm_val = 0;
    /*clear the compare block*/
    timer_clear(RTT_DEV, RTT_CHAN);
}

void rtt_poweron(void)
{
	timer_start(RTT_DEV);
}

void rtt_poweroff(void)
{
	timer_stop(RTT_DEV);
}

void rtt_isr_cb(void *arg, int channel)   //typedef void (*timer_cb_t)(void *arg, int channel);
{
	(void)arg;
	/* the trick is that channel is a signed int with -1 meaning overflow */
    if (channel < 0) {
        if (rtt_state.overflow_cb != NULL) {
            rtt_state.overflow_cb(rtt_state.overflow_arg);
        }
    }
    else
    {
        if (rtt_state.alarm_cb != NULL) {
            rtt_state.alarm_cb(rtt_state.alarm_arg);
        }
    }
}

#endif /*USE_PERIPH_CORTUS_RTT*/
