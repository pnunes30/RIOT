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
 * @brief       Implementation of the kernel's architecture dependent thread interface
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */

/*
 *
 * APS CPU use the following register layout when saving their context onto the stack:
 *
 * -------- highest address (bottom of stack)
 * | IRQ  | --> interrupt mask ic->irq (interrupt pending ?)
 * --------
 * | PSR  |
 * --------
 * | RTT  |
 * --------
 * | R15  |
 * --------
 * | R14  |
 * --------
 * | R13  |
 * --------
 * | R12  |
 * --------
 * | R11  |
 * --------
 * | R10  |
 * --------
 * | R9   |
 * --------
 * | R8   |
 * --------
 * | R7   |
 * --------
 * | R6   |
 * --------
 * | R5   |
 * --------
 * | R4   |
 * --------
 * | R3   |
 * --------
 * | R2   |
 * -------- lowest address (top of stack)
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @}
 */

#include <stdio.h>

#include "sched.h"
#include "thread.h"
#include "irq.h"
#include "cpu.h"

//extern uint32_t _estack;
//extern uint32_t _sstack;

#define IRQ_TRAP_YIELD              31

/* The initial PSR has the Previous Interrupt Enabled (PIEN) flag set. */
#define INITIAL_PSR                 (0x00020000)
/**
 * @brief   Noticeable marker marking the beginning of a stack segment
 *
 * This marker is used e.g. by *cpu_switch_context_exit* to identify the
 * stacks beginning.
 */
#define STACK_MARKER                (0x77777777)

static void __context_save(void);
static void __context_restore(void);

char *thread_stack_init(thread_task_func_t task_func,
                             void *arg,
                             void *stack_start,
                             int stack_size)
{
    uint32_t *stk;
    stk = (uint32_t *)((uintptr_t)stack_start + stack_size);

    /* adjust to 32 bit boundary by clearing the last two bits in the address */
    stk = (uint32_t *)(((uint32_t)stk) & ~((uint32_t)0x3));

    /* stack start marker */
    stk--;
    *stk = STACK_MARKER;

    /* Space for pending IRQ flag saving. */
    stk--;
    *stk = 0;

    /* Initial PSR */
    stk--;
    *stk = INITIAL_PSR;

    /* RTT contains the thread entry function */
    stk--;
    *stk = (uint32_t)task_func;

    /* R15 - contains the return address when the thread exits */
    stk--;
    *stk = (uint32_t)sched_task_exit; //--> TO CONFIRM

    // R14 set to 0?

    /* Space for registers. */
    for (unsigned int i = 14; i > 2; i--) {
        *stk = i << 24 | i << 16 | i << 8 | i;
        --stk;
    }

    /* r2 - contains the thread function parameter */
    stk--;
    *stk = (uint32_t)arg;

    return (char*) stk;
}

void thread_stack_print(void)
{
    int count = 0;
    uint32_t *sp = (uint32_t *)sched_active_thread->sp;

    printf("printing the current stack of thread %" PRIkernel_pid "\n",
           thread_getpid());
    printf("  address:      data:\n");

    do {
        printf("  0x%08x:   0x%08x\n", (unsigned int)sp, (unsigned int)*sp);
        sp++;
        count++;
    } while (*sp != STACK_MARKER);

    printf("current stack size: %i byte\n", count);
}

/* This function returns the number of bytes used on the ISR stack */
int thread_isr_stack_usage(void)
{
    /* TODO */
    return -1;
}

void *thread_isr_stack_pointer(void)
{
    /* TODO */
    return (void *)-1;
}

void *thread_isr_stack_start(void)
{
    /* TODO */
    return (void *)-1;
}

__attribute__((naked)) void NORETURN cpu_switch_context_exit(void)
{
    sched_run();

    irq_enable();
    __context_restore();
    __asm__ volatile("rti");  //  Not sure that we in an ISR context ?

    UNREACHABLE();
}

// /* trigger a SW interrupt to run scheduler and schedule new thread if applicable */
#define thread_yield_higher(void)    asm __volatile__( " trap #%0 "::"i"(IRQ_TRAP_YIELD):"memory")

__attribute__((always_inline)) static inline void __context_save(void)
{
    __asm__ volatile(
        "sub	r1, #68					\n"	/* Make space on the stack for the context. */							\
        "std	r2, [r1] + 	0			\n"																			\
        "stq	r4, [r1] +	8			\n"																			\
        "stq	r8, [r1] +	24			\n"																			\
        "stq	r12, [r1] +	40			\n"																			\
        "mov	r6, rtt					\n"																			\
        "mov	r7, psr					\n"																			\
        "std	r6, [r1] +	56			\n"																			\
        "movhi	r2, #16384				\n"	/* Set the pointer to the IC. */										\
        "ldub	r3, [r2] + 2			\n"	/* Load the current interrupt mask. */									\
        "st		r3, [r1]+ 64			\n"	/* Store the interrupt mask on the stack. */ 							\
        "ld		r2, [r0]+short(sched_active_thread)	\n"	/* Load the pointer to the current TCB. */					\
        "st		r1, [r2]				\n"	/* Save the stack pointer into the TCB. */								\
        /*"mov	r14, r1					\n"*//* Compiler expects r14 to be set to the function stack. */				\
    	);
}


__attribute__((always_inline)) static inline void __context_restore(void)
{
    __asm__ volatile(
        "ld		r2, [r0]+short(sched_active_thread)	\n"	/* load address of current tcb to find the stack pointer and context. */		\
        "ld		r1, [r2]				\n"																			\
        "movhi	r2, #16384				\n"	/* Set the pointer to the IC. */										\
        "ld		r3, [r1] + 64			\n"	/* Load the previous interrupt mask. */									\
        "stb	r3, [r2] + 2  			\n"	/* Set the current interrupt mask to be the previous. */				\
        "ldd	r6, [r1] + 56			\n"	/* Restore context. */													\
        "mov	rtt, r6					\n"																			\
        "mov	psr, r7					\n"																			\
        "ldd	r2, [r1] + 0			\n"																			\
        "ldq	r4, [r1] +	8			\n"																			\
        "ldq	r8, [r1] +	24			\n"																			\
        "ldq	r12, [r1] +	40			\n"																			\
        "add	r1, #68					\n"																			\
//        "rti							\n"
    );
}


/* Trap 31 handler. */
void interrupt31_handler( void ) __attribute__((naked));
void interrupt31_handler( void )
{
    __context_save();

    /* have sched_active_thread point to the next thread */
    sched_run();

    __context_restore();
    __asm__ volatile("rti");

    UNREACHABLE();
}
