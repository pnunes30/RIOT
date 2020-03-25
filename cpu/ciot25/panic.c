/*
 * panic.c
 *
 *  Created on: Mar 28, 2018
 *      Author: cortus
 *
 *      Imported from cortex_m
 */




#include "cpu.h"
#include "log.h"

#if 0
#ifdef DEVELHELP
static void print_ipsr(void)
{
    uint32_t ipsr = __get_IPSR() & IPSR_ISR_Msk;
    if(ipsr) {
        /* if you get here, you might have forgotten to implement the isr
         * for the printed interrupt number */
        LOG_ERROR("Inside isr %d\n", ((int)ipsr) - 16);
    }
}
#endif
#endif

void panic_arch(void)
{
	assert_break();
#ifdef DEVELHELP
    //print_ipsr();
    /* The bkpt instruction will signal to the debugger to break here. */
    __asm__ volatile ("trap #3\n");
    do {} while (1);
#endif
}
