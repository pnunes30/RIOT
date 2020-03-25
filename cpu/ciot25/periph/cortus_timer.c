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

#ifdef __APS__
#pragma GCC optimize ("O3") /* critical inlines must be ensured for accurate behaviour */
#endif

#include "cpu.h"
#include "periph_conf.h"
#include "periph/timer.h"
#include "periph_cpu.h"
#include "board.h"

#include <math.h>

#ifdef USE_PERIPH_CORTUS_TIMER

#define ENABLE_DEBUG (0)
#include "debug.h"

#ifdef APS_TIMER_OVERFLOW0
volatile uint32_t   aps_timer_overflow_cnt=0;
#endif

#ifdef MODULE_XTIMER
#include "xtimer.h"
#ifdef MODULE_STDIO_UART
#include "stdio_uart.h"
#endif
#endif



/**
 * @brief   Interrupt context for each configured timer
 */
static timer_isr_ctx_t isr_ctx[TIMER_NUMOF];

#ifdef _XTIMER_FIX_9_MASK_NOW_
static uint32_t _isr_tick[TIMER_NUMOF];
#endif


static inline uint32_t timer_frequency(uint8_t clk, uint8_t pre)
{
    return ( clock_source(clk) / timer_prescaler_value(pre) );
}

/**
 * @brief   Get the timer device
 */
static inline Timer *dev(tim_t tim)
{
    return timer_config[tim].dev;
}


#ifdef APS_TIMER_CALIBRATE
volatile unsigned int fired;
unsigned int aps_xtimer_backoff=0;
unsigned int aps_xtimer_overhead=0;
unsigned int aps_xtimer_isr_backoff=0;


static void _timer_calibrate_cb(void *arg, int chan)
{
    (void)chan;
    fired=timer_read((tim_t)arg);
}

void timer_calibrate(tim_t tim, uint8_t clk, uint8_t pre)
{
    unsigned int chan=0;
    unsigned int diff=0;
    unsigned int val0, val1, val2, val3, val4;
    unsigned int loop1cnt=0;
    unsigned int timer_read_overhead=0;
    unsigned int timer_isr_overhead=0;

    /* use fake irq callback */
    isr_ctx[tim].cb  = _timer_calibrate_cb;
    isr_ctx[tim].arg = (void*)tim; //(void *)&fired;

#define TIMCAL_LOOPCNT 1000
#define TIMCAL_RUNCNT 10
    dev(tim)->tclk_sel  = clk;
    dev(tim)->prescaler = pre;
    unsigned int flags = irq_enable();
    timer_start(tim);
    for (int z=0; z<TIMCAL_RUNCNT; z++)
    {
        /* read overhead & fixed loop */
        fired=0;
        val0 = timer_read(tim);
        val1 = timer_read(tim);
        while (fired<TIMCAL_LOOPCNT) { fired++; };
        val2 = timer_read(tim);
        diff = val2-val1;

        /* isr overhead */
        fired=0;
        loop1cnt=0;
        val3 =(unsigned int)timer_set(tim, chan, diff);
        while (fired<=0 && loop1cnt<2*TIMCAL_LOOPCNT) { loop1cnt++; };
        val4 = fired;

        timer_read_overhead += val1-val0;
        timer_isr_overhead  += val4-val3-(val2-val1)-(val1-val0);
    }
    timer_stop(tim);
    irq_restore(flags);

    timer_read_overhead = timer_read_overhead/TIMCAL_RUNCNT;
    timer_isr_overhead  = timer_isr_overhead/TIMCAL_RUNCNT;

    aps_xtimer_overhead      =_CAL_XTIMER_OVERHEAD(timer_read_overhead, timer_isr_overhead);
    aps_xtimer_backoff       =_CAL_XTIMER_BACKOFF(timer_read_overhead, timer_isr_overhead);
    aps_xtimer_isr_backoff   =_CAL_XTIMER_ISR_BACKOFF(timer_read_overhead, timer_isr_overhead);

    printf("APS Timer calibration[%d,%d] constants:"
            " raw(freq=%8" PRIu32
            " read=%3" PRIu32
            " isr=%3" PRIu32 ")"
            " overhead=%4"    PRIu32 " (%2" PRIu32 "/MHz)"
            " backoff=%4"     PRIu32 " (%2" PRIu32 "/MHz)"
            " isr-backoff=%4" PRIu32 " (%2" PRIu32 "/MHz)"
            "\n", clk, pre,
            (timer_frequency(clk,pre)),
            (uint32_t)(timer_read_overhead),
            (uint32_t)(timer_isr_overhead),
            (uint32_t)(aps_xtimer_overhead),    (uint32_t)(aps_xtimer_overhead/(timer_frequency(clk,pre)/1000000.0)),
            (uint32_t)(aps_xtimer_backoff),     (uint32_t)(aps_xtimer_backoff/(timer_frequency(clk,pre)/1000000.0)),
            (uint32_t)(aps_xtimer_isr_backoff), (uint32_t)(aps_xtimer_isr_backoff/(timer_frequency(clk,pre)/1000000.0))
            );
}
#endif /*APS_TIMER_CALIBRATE*/

int timer_init(tim_t tim, unsigned long freq, timer_cb_t cb, void *arg)
{
    /* check if device is valid */
    if (tim >= TIMER_NUMOF) {
        return -1;
    }

    dev(tim)->enable = 0;

    /* !! RIOT expects XTIMER_HZ (normally 1MHz) native timer clock. !!
     * Check if the static timer config matches the requested freq,
     */
    /* check if the freq and width config is correct */
#if defined(APS_XTIMER_OVERCLOCK) || ((ARCH_XTIMER_HZ != XTIMER_HZ) && !defined(ARCH_XTIMER_CONVERSION))
#warning   "WARNING: Real timer frequency will not match expected xtimer frequency. Your system may run faster or slower than expected";
    printf("WARNING: Real timer frequency does not match expected xtimer frequency. Your system may run faster or slower than expected\n");
    aps_assert( (ARCH_XTIMER_HZ == timer_frequency(timer_config[tim].clk, timer_config[tim].pre)) );
#else
    assert( (timer_frequency(timer_config[tim].clk, timer_config[tim].pre) == freq) );
#endif

    /* initialize the timer main block */
    dev(tim)->tclk_sel  = timer_config[tim].clk;
    dev(tim)->prescaler = timer_config[tim].pre;
    dev(tim)->period    = timer_config[tim].max;

    /* enable the timer overflow interrupt if required (disabled by default) */
    if (timer_config[tim].irqo>0) {
    	dev(tim)->mask      = 1;
    	aps_irq[timer_config[tim].irqo].ipl = 0;
    	aps_irq[timer_config[tim].irqo].ien = 1;
    }else{
    	dev(tim)->mask      = 0;
    }

    /* initialize the compare blocks and irqs, disabled for now */
    Timer_Cmp *cmp;
    for( uint8_t channel = 0; channel < TIMER_CHAN; channel++)
    {
        cmp = timer_config[tim].cmp[channel];
        cmp->status = 0;
        cmp->enable = 0;

        /* enable compare irqs lines once in IC, without effect as long as mask is not set */
        if (timer_config[tim].irqn[channel]>0) {
        	aps_irq[timer_config[tim].irqn[channel]].ipl = 0;
        	aps_irq[timer_config[tim].irqn[channel]].ien = 1;
        }
    }

    /* initialize the capture block in software capture mode */
    Timer_Cap *cap = timer_config[tim].cap;
    cap->enable = 0;
    cap->status = 0;
    cap->edge_sel = 0x0;    //rising edge
    cap->mask = 0;          //no interrupts
    cap->capture = 0;       //reset to known state
    cap->in_cap_sel = 0x4;  //software capture
    cap->enable  = 1;       //ready to capture

#ifdef APS_TIMER_CALIBRATE
    /* run calibration tests on channel 0 */
    {
#if 1
        for (uint8_t clk=0; clk<=3; clk++) {
            for (uint8_t pre=0; pre<=3; pre++) {
                timer_calibrate(tim, clk, pre);
            }
        }
#endif
        timer_calibrate(tim, timer_config[tim].clk, timer_config[tim].pre);
        isr_ctx[tim].cb  = cb;
        isr_ctx[tim].arg = arg;
    }
#endif

#if 1 /*APS_DEBUG*/
    printf("APS Timer initialized:"
            " ext freq=%" PRIu32
            " clk freq=%" PRIu32
            " clk_pre=1/%" PRIu32
            " period=%" PRIx32
            " xoverhead=%4"    PRIu32
            " xbackoff=%4"     PRIu32
            " xisr-backoff=%4" PRIu32
            "\n",
            (uint32_t)freq,
            (uint32_t)clock_source(timer_config[tim].clk),
            (uint32_t)timer_prescaler_value(timer_config[tim].pre),
            (uint32_t)timer_config[tim].max,
            (uint32_t)(XTIMER_OVERHEAD),
            (uint32_t)(XTIMER_BACKOFF),
            (uint32_t)(XTIMER_ISR_BACKOFF)
            );
#endif

    /* Now, enable real system irq callback */
    isr_ctx[tim].cb  = cb;
    isr_ctx[tim].arg = arg;

    /* reset the counter and start the timer */
    timer_start(tim);

#if defined(MODULE_UART_STDIO) && defined(MODULE_XTIMER) && (APS_UART_TX_WAIT_ISR != 0)
    if (tim == XTIMER_DEV) {
        uart_init_tx_isr(STDIO_UART_DEV);
    }
#endif

    return 0;
}


static inline unsigned int _timer_read_raw(tim_t tim)
{
    /*
     * WARNING: The capture block must be enabled (enable==1) and idle (capture=0)
     *          when entering here. Be careful about the order of the sfr manipulation.
     *
     * HW limitation1: we must wait 6 *timer input* clock cycles between 2 captures (cf doc).
     *                 Depending on the cpu_freq/timer_freq ratio, we might need to wait
     *                 in the status check.
     * HW limitation2: the cap->capture toggling 1->0->1 must be wider than a
     *                 timer clock pulse to be seen by the timer hw.
     * Workarounds: retry loop + guard loop + assume we enter here with cap->capture=0.
     *              If retries happen too often, you'll need to review your timer config
     *              and/or activate the guard loop unconditionally, iow before status loop.
     * "Not a bug, just a feature..."
     * -jmhe-
     */
    volatile Timer_Cap *cap = timer_config[tim].cap;
    uint32_t value=0;
    uint8_t loop_cnt=0;
    uint8_t loop_try=0;

    /* capture block only works if timer block is enabled. assert or return 0? */
    //assert(dev(tim)->enable);
    if      (dev(tim)->enable)
    {
        aps_atomic_start();
retry:
        cap->capture = 1;
        while(!cap->status) {
#define APS_MAX_CAP_LOOP 50
            if (loop_cnt++>APS_MAX_CAP_LOOP) {
            	cap->capture = 0;
                loop_cnt=0;
                loop_try++;
                assert(loop_try<3);
                printf("WARNING: capture timeout [%d]. review your timer config\n",loop_try);
                for (unsigned int i=0; i<=(CLOCK_FREQUENCY/timer_frequency(dev(tim)->tclk_sel, dev(tim)->prescaler)); i+=2/*inst*/) {}; /* guard loop */
                goto retry;
            }
        }
        value = cap->value;
        cap->capture = 0;
        cap->status  = 0;

#ifdef APS_DEBUG
        __aps_debug.tim_last_cap_loop_cnt=loop_cnt+APS_MAX_CAP_LOOP*loop_try;
        __aps_debug.tim_max_cap_loop_cnt=MAX(__aps_debug.tim_max_cap_loop_cnt,__aps_debug.tim_last_cap_loop_cnt);
#ifdef APS_XTIMER_DEBUG
        if (tim == ARCH_XTIMER_DEV) {
            __aps_debug.tim_last_cap_value=value;
            __aps_debug.tim_last_cap_pid=sched_active_thread->pid;
        }
#endif
#endif

        aps_atomic_end();
    }

    return (unsigned int)value;
}


inline unsigned int timer_read(tim_t tim)
{
    return _timer_read_raw(tim);
}

static inline unsigned int _timer_set(tim_t tim, int channel, unsigned int timeout, int flg_absolute)
{
    uint32_t raw_ref;
    uint32_t raw_now;
    uint32_t raw_timeout;

    assert (channel < (int)TIMER_CHAN);
    aps_assert ( timeout <= timer_config[tim].max );    /*sanity check*/

    Timer_Cmp *cmp = timer_config[tim].cmp[channel];
    
    /* in relative mode, we refuse to be interrupted between capture and offset.*/
    aps_atomic_start();

#ifdef APS_XTIMER_DEBUG
    if (tim == ARCH_XTIMER_DEV) {
        ___inc(timset);
        ___dbg(timset).prvcmp=cmp->compare;
        ___dbg(timset).prvcap=__aps_debug.tim_last_cap_value;
        ___dbg(timset).prvpid=__aps_debug.tim_last_cap_pid;
    }
#endif

    raw_ref     = _timer_read_raw(tim);
    raw_now     = (flg_absolute) ? 0 : raw_ref;
    raw_timeout = timeout;

    /* HW limitation3: the compare block cannot compare on the 'period' value.
     *                 iow 0xffffffff or whatever is not comparable
     *                 => must compare with 0
     * In relative mode, we handle overflow whatever the width.
     */
    if (raw_timeout >= (uint32_t)(timer_config[tim].max - raw_now)) {
        raw_timeout -= (uint32_t)(timer_config[tim].max - raw_now);
    }else{
        raw_timeout += (uint32_t)(raw_now);
    }

#ifdef APS_XTIMER_DEBUG
    /* xtimer should never replace a pending timer */
    /* WARNING: potential race condition: 'set' is not 'replace'.
     *          We refuse to mask a pending irq by setting status=0 or enable=0.
     *  //cmp->enable = 0;
     *  //cmp->status = 0;
     * Instead we check and assert race conditions that
     * might indicate a bug in the higher implementation.
     */
    if (tim == ARCH_XTIMER_DEV)
        aps_assert(cmp->status == 0);
#endif

    cmp->mask    = 1;
    cmp->compare = raw_timeout;
    cmp->enable  = 1;


#ifdef APS_XTIMER_DEBUG
    if (tim == ARCH_XTIMER_DEV) {
        ___dbg(timset).setcmp = raw_timeout;
        ___dbg(timset).settim = raw_ref;
        ___dbg(timset).setpid = sched_active_thread->pid;
        ___dbg(timset).setabs = flg_absolute;
        ___dbg(timset).longc  = _xtimer_period_cnt;
        /* xtimer should never set a timer in the past or in the next period
         * If this assert fails, check your XTIMER_OVERHEAD constant */
        aps_assert ( (timeout == ARCH_XTIMER_MAX) || (!flg_absolute) || (raw_ref <= raw_timeout) );
    }
#endif

    aps_atomic_end();

    return (unsigned int)raw_ref;
}


unsigned int timer_set_absolute(tim_t tim, int channel, unsigned int timeout)
{
    return _timer_set(tim, channel, timeout, 1);
}

#ifdef PERIPH_TIMER_PROVIDES_SET /* use our own definition instead of common one*/
unsigned int timer_set(tim_t tim, int channel, unsigned int offset)
{
    return _timer_set(tim, channel, offset, 0);
}
#endif


int timer_clear(tim_t tim, int channel)
{
    assert(channel < (int)TIMER_CHAN);

    Timer_Cmp *cmp = timer_config[tim].cmp[channel];
    aps_atomic_start();
    cmp->enable = 0;
    cmp->status = 0;
    cmp->mask = 0;
    aps_atomic_end();

    return 0;
}

void timer_start(tim_t tim)
{
#ifdef APS_TIMER_OVERFLOW0
    if (tim==0) aps_timer_overflow_cnt=0;
#endif
    dev(tim)->enable = 1;
}

void timer_stop(tim_t tim)
{
    dev(tim)->enable = 0;

    for( uint8_t channel = 0; channel < TIMER_CHAN; channel++)
    {
        timer_clear(tim, channel);
    }
}

#ifdef _XTIMER_FIX_9_MASK_NOW_
uint32_t timer_get_isr_tick(unsigned int tim)
{
    return _isr_tick[tim];
}
#endif

static inline void timer_irq_handler(tim_t tim, int channel)
{
	/* the trick is that channel is a signed int with -1 meaning overflow */
	uint32_t status;
	if (channel < 0) /* overflow */
	{
#ifdef APS_DEBUG
		++(__aps_debug.irq_tim_overflow);
#endif
		status = timer_config[tim].dev->status;
		assert( status != 0);
		/* FIXME: one shot? clear overflow interrupts ? */
	}
	else
	{
#ifdef APS_DEBUG
		++(__aps_debug.irq_tim_cmp);
#endif
		status = timer_config[tim].cmp[channel]->status;
		assert( (channel < (int)TIMER_CHAN) );
		assert( status != 0);
#ifdef _XTIMER_FIX_9_MASK_NOW_
		_isr_tick[tim] = timer_config[tim].cmp[channel]->compare;
#endif
		/* one shot ISR: disable channel interrupts & reset status */
		timer_clear(tim, channel);
	}

	if (status && isr_ctx[tim].cb) {
        isr_ctx[tim].cb(isr_ctx[tim].arg, channel);
    }
}


#ifdef IRQ_TIMER1
void interrupt_handler(IRQ_TIMER1)
{
#ifdef APS_TIMER_OVERFLOW0
	//internal overflow isr dedicated to XTIMER_DEV (==0).
    //this is an internal arch interrupt. not riot related. no reschedule
    //__enter_isr();
    dev(0)->status=0;
    ++aps_timer_overflow_cnt;
# ifdef APS_DEBUG
    __aps_debug.irq_tim_overflow=aps_timer_overflow_cnt;
#  ifdef APS_XTIMER_DEBUG
    __aps_debug.tim_overflow_last_cmp=timer_config[ARCH_XTIMER_DEV].cmp[0]->compare;

    //_long_cnt is only incremented in isr. give it a chance to be incremented here.
    //but it is not an error if the diff is 1, as _long_cnt can be incremented
    //in the next period (xtimer_set_absolute with now=0xfffffffe).
    //but this is an error if diff>1
    int status = irq_enable();
    /*xtimer oveflow isr*/
    irq_restore(status);
    if (_xtimer_period_cnt != aps_timer_overflow_cnt) {
        __aps_debug.tim_overflow_err_cnt++;
        aps_assert(aps_timer_overflow_cnt-_xtimer_period_cnt<=1);
    }
#  endif
# endif
#else
    __enter_isr();
    timer_irq_handler(0, -1); /*overflow*/
    __exit_isr();
#endif /*APS_TIMER_OVERFLOW*/
}
#endif /*IRQ_TIMER1*/

#ifdef IRQ_TIMER2
void interrupt_handler(IRQ_TIMER2)
{
    __enter_isr();
    timer_irq_handler(1, -1); /*overflow*/
    __exit_isr();

}
#endif /*IRQ_TIMER2*/


#ifdef IRQ_TIMER1_CMPA
void interrupt_handler(IRQ_TIMER1_CMPA)
{
    __enter_isr();
    timer_irq_handler(0, 0);
    __exit_isr();
}
#endif

#ifdef IRQ_TIMER1_CMPB
void interrupt_handler(IRQ_TIMER1_CMPB)
{
    __enter_isr();
    timer_irq_handler(0, 1);
    __exit_isr();
}
#endif

#ifdef IRQ_TIMER2_CMPA
void interrupt_handler(IRQ_TIMER2_CMPA)
{
    __enter_isr();
    timer_irq_handler(1, 0);
    __exit_isr();
}
#endif

#ifdef IRQ_TIMER2_CMPB
void interrupt_handler(IRQ_TIMER2_CMPB)
{
    __enter_isr();
    timer_irq_handler(1, 1);
    __exit_isr();
}
#endif


#endif /*USE_PERIPH_CORTUS_TIMER*/
