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
 * ------- lowest address (bottom of stack)
 * | R2  | <-- R1=sched_thread->sp   after context_save (sub (17*4=0x44))
 * | R3  |
 * | R4  |
 * | R5  |_
 * | R6  |
 * | R7  |
 * | R8  |
 * | R9  |_
 * | R10 |
 * | R11 |
 * | R12 |
 * | R13 |_
 * | R14 |
 * | R15 |
 * | RTT |
 * | PSR |_
 * | --- |  UNUSED: was interrupt mask 'ic->irq' (interrupt pending ?), used for DBG
 * ------- higher address (top of stack)
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

/* The initial PSR has the Previous Interrupt Enabled (PIEN) flag set. 
 * It MUST have IEN=0 to avoid corruption during initial restore.
 */
#define INITIAL_PSR                 (0x000E0000)
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
    *(--stk) = STACK_MARKER;

    /* Space for pending IRQ flag saving. */
    *(--stk) = 0;

    /* Initial PSR */
    *(--stk) = INITIAL_PSR;

    /* RTT contains the thread entry function */
    *(--stk) = (uint32_t)task_func;

    /* R15 - contains the return address when the thread exits */
    *(--stk) = (uint32_t)sched_task_exit; //--> TO CONFIRM

    // R14 set to 0?

    /* Space for registers. */
    for (unsigned int i = 14; i > 2; i--) {
        *(--stk) = i << 24 | i << 16 | i << 8 | i;
    }

    /* r2 - contains the thread function parameter */
    *(--stk) = (uint32_t)arg;

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

    //irq_enable();
    //__exit_isr();
    __context_restore();

    UNREACHABLE();
}

/* Trigger a SW interrupt to run scheduler and schedule new thread if applicable.
 * (software traps are not maskable by IEN=0)
 */
void thread_yield_higher(void)
{
    asm __volatile__( " trap #%0 "::"i"(IRQ_TRAP_YIELD):"memory");
}

__attribute__((always_inline)) static inline void __context_save(void)
{
	/* nothing before */
    __asm__ volatile(
            "sub    r1, #0x44               \n" /* Make space on the stack for the context. */
            "std    r2, [r1] +  0x0         \n"
            "stq    r4, [r1] +  0x8         \n"
            "stq    r8, [r1] +  0x18        \n"
            "stq    r12,[r1] +  0x28        \n"
            "mov    r6, rtt                 \n"
            "mov    r7, psr                 \n"
            "std    r6, [r1] +  0x38        \n"
#ifdef APS_DEBUG
            /*"movhi    r2, #16384          \n" *//* Set the pointer to the IC. */
            /*"ldub r3, [r2] + 2            \n" *//* Load the current interrupt mask. */
            /*"st       r3, [r1]+ 0x40      \n" *//* Store the interrupt mask on the stack. */
    		"ld     r2, [%0]                \n"
    		"add	r2,#0x1					\n"	//++(__aps_debug.ctx_save);
    		"st     r2, [%0]                \n"
    		"st     r2, [r1]+   0x40        \n" /* Store the ctx_save counter to track sequentiality. */
#endif
            "ld     r2, [r0]+short(sched_active_thread) \n" /* Load the pointer to the current TCB. */
            "st     r1, [r2]                \n" /* Save the stack pointer into the TCB. */
            /*"mov  r14, r1                 \n"*//* Compiler expects r14 to be set to the function stack. */
#ifdef APS_DEBUG
            ::"a" (&__aps_debug.ctx_save)
#endif
        );
}


__attribute__((always_inline)) static inline void __context_restore(void)
{
#ifdef APS_DEBUG
    ++(__aps_debug.ctx_restore);
#endif
    irq_disable(); 
    __asm__ volatile(
        "ld     r2, [r0]+short(sched_active_thread) \n" /* load address of current tcb to find the stack pointer and context. */        \
        "ld     r1, [r2]                \n"
#ifdef APS_DEBUG
        "st     r0, [r2]                \n" /* INVALIDATE the stack pointer into the TCB for gdbinit macros and bug detection. */
        /*"movhi    r2, #16384          \n" *//* Set the pointer to the IC. */
        /*"ld       r3, [r1] + 0x40     \n" *//* Load the previous interrupt mask. */
        /*"stb  r3, [r2] + 2            \n" *//* Set the current interrupt mask to be the previous. */
#endif
        "ldd    r6, [r1] + 0x38         \n" /* Restore context. */
        "mov    rtt, r6                 \n"
        "mov    psr, r7                 \n" /* IEN must be 0 when restoring! IEN was disabled in context_save but not in cpu_context_switch */
        "ldd    r2, [r1] + 0x0          \n"
        "ldq    r4, [r1] + 0x8          \n"
        "ldq    r8, [r1] + 0x18         \n"
        "ldq    r12,[r1] + 0x28         \n"
        "add    r1, #0x44               \n"

        "rti                            \n" /* activate this restored context, set PIEN=1*/
    );
    /* nothing after */
}


/* Trap 31 handler. */
void interrupt31_handler( void ) __attribute__((naked));
void interrupt31_handler( void )
{
    __context_save();

#ifdef APS_DEBUG
    (__aps_debug.irq_31)++;
#endif

    /* have sched_active_thread point to the next thread */
    sched_run();

    __context_restore();

    UNREACHABLE();
}
