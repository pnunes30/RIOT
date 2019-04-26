/*
 * Copyright (C) 2014-2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    cpu_aps Cortus APS
 * @ingroup     cpu
 * @brief       Common implementations and headers for APS family based
 *              micro-controllers
 * @{
 *
 * @file
 * @brief       Basic definitions for the Cortus APS CPU
 *
 * When ever you want to do something hardware related, that is accessing MCUs
 * registers, just include this file. It will then make sure that the MCU
 * specific headers are included.
 *
 * @author      Stefan Pfeiffer <stefan.pfeiffer@fu-berlin.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Joakim Nohlgård <joakim.nohlgard@eistec.se>
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @todo        remove include irq.h once core was adjusted
 */

#ifndef CPU_H
#define CPU_H

#include <sys/types.h>	//mini/newlib fixes
#include "irq.h"
#include "sched.h"
#include "thread.h"
#include "cpu_conf.h"

#ifdef __cplusplus
extern "C" {
#endif


/***************************** GLOBAL FLAGS **********************************/

/* APS_DEBUG defined in cpu_conf.h */
#undef APS_XTIMER_DEBUG			/* requires APS_DEBUG */

#define MODULE_STDIO_UART   1	/* activated in cpu/makefile (USEMODULE += stdio_uart) */
/*****************************************************************************/

#ifdef APS_DEBUG
#ifdef APS_XTIMER_DEBUG

#define APS_MAX_HIST 4

/* volatile to force systematic compiler ld/st */
typedef struct {
	volatile uint8_t    idx;
	struct {
		volatile uint32_t   listh;
		volatile uint32_t   cmpar;
		volatile uint32_t   refer;
		volatile uint32_t   targt;
		volatile int32_t    tleft;
		volatile uint32_t   longc;
		volatile uint32_t   shoot;
		volatile uint32_t   nextt;
	} a[APS_MAX_HIST];
} aps_xtimcb_t;
typedef struct {
	volatile uint8_t    idx;
	struct {
		volatile uint32_t   prvcmp;
		volatile uint32_t   prvcap; /* last read before set */
		volatile uint32_t   setcmp;
		volatile uint32_t   settim;
		volatile uint32_t   setpid;
		volatile uint32_t   prvpid; /* last read before set */
		volatile uint32_t   setabs;
		volatile uint32_t   longc;
	} a[APS_MAX_HIST];
} aps_timset_t;

#define ___idx(v)  __aps_debug.v.idx
#define ___inc(v)  if (++___idx(v) >= APS_MAX_HIST) ___idx(v)=0
#define ___dbg(v)  __aps_debug.v.a[__aps_debug.v.idx]
#define aps_timer_cmp0 timer_config[0].cmp[0]->compare;

#endif /*APS_XTIMER_DEBUG*/

typedef struct {
    volatile uint32_t   irq_31;
    volatile uint32_t   irq_cnt;
    volatile uint32_t   irq_tim_overflow;
    volatile uint32_t   irq_tim_cmp;
    volatile uint32_t   irq_uart1_rx;
    volatile uint32_t   irq_uart1_tx;
    volatile uint32_t   irq_uart2_rx;
    volatile uint32_t   irq_uart2_tx;
    volatile uint32_t   irq_nested;
    volatile uint32_t   ctx_save;
    volatile uint32_t   ctx_restore;
    volatile uint32_t   tim_last_cap_loop_cnt;
    volatile uint32_t   tim_max_cap_loop_cnt;
    volatile uint32_t   uart_max_loop_cnt;
    volatile uint32_t	uart_tx_timeout_cnt;
#ifdef APS_XTIMER_DEBUG
    volatile aps_xtimcb_t xtimcb;
    volatile aps_timset_t timset;
    volatile uint32_t   tim_overflow_err_cnt;
    volatile uint32_t   tim_overflow_last_cmp;
    volatile uint32_t   tim_last_cap_value;
    volatile uint32_t   tim_last_cap_pid;
    volatile uint32_t   tim_xtimer_setab;
    volatile uint32_t   tim_xtimer_set32;
    volatile uint32_t   tim_xtimer_set64;
    volatile uint32_t   tim_xtimer_shoot;
    volatile uint32_t   tim_xtimer_list_ad0;
    volatile uint32_t   tim_xtimer_list_ad1;
    volatile uint32_t   tim_xtimer_list_adv;
    volatile uint32_t   tim_xtimer_list_cnt;
    volatile uint32_t   tim_xtimer_list_max;
#endif /*APS_XTIMER_DEBUG*/
} aps_debug_t;

extern aps_debug_t __aps_debug;


/* DEBUG: use 'aps_assert' with a breakpoint on 'assert_break'
 * to have a meaning full backtrace when assert fails and be
 * able to dump_trace before printf.
 * assert() is redefined to bypass core_panic() which trashes the stack.
 * A 'continue' will still be possible, the cpu will not be stopped. */
void assert_break(void);
void debug_break(int i);
#define aps_assert(cond) if (!(cond)) { assert_break(); printf("ASSERT FAILED: %s:%d (",__FILE__, __LINE__); printf(__STRINGIFY(cond)); printf(")\n"); }
//#ifdef assert
//#undef assert
//#endif
//#define assert(cond) aps_assert(cond)
#else /*APS_DEBUG*/
#undef  APS_XTIMER_DEBUG
#define aps_assert(cond)
#define assert_break()	/*nothing*/
#define __aps_debug     (void)0
#endif /*APS_DEBUG*/


/**
 * @brief Atomic blocks macros. They trigger a compilation error if not paired.
 */
#define aps_atomic_start()  do { unsigned int _aps_irq_flags = irq_disable();
#define aps_atomic_end()         irq_restore(_aps_irq_flags); } while(0);




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

/**
 * @brief global in-ISR state variable
 */
extern volatile uint8_t __in_isr;

/**
 * @brief Flag entering of an ISR
 */
static inline void __enter_isr(void)
{
    ++__in_isr;// = 1;				//allow nested interrupts (i.e. ethos)
#ifdef APS_DEBUG
    __aps_debug.irq_cnt++;
    __aps_debug.irq_nested=MAX(__aps_debug.irq_nested, __in_isr);
#endif
}

/**
 * @brief Flag exiting of an ISR
 */
static inline void __exit_isr(void)
{
    --__in_isr;// = 0;				//allow nested interrupts (i.e. ethos)
    aps_isr_end();
}

/**
 * @brief   Initialization of the CPU
 */
void cpu_init(void);

/**
 * @brief   Prints the current content of R15
 */
static inline void cpu_print_last_instruction(void)
{
    uint32_t *lr_ptr;
    __asm__ __volatile__("mov %0, r15\n" : "=r"(lr_ptr));
    printf("%p\n", (void*) lr_ptr);
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


#ifdef __cplusplus
}
#endif

#endif /* CPU_H */
/** @} */
