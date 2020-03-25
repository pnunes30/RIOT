/*********************************************************************************
 * This confidential and proprietary software may be used only as authorized 
 *                      by a licensing agreement from                           
 *                           Cortus S.A.S.
 *
 *                 (C) Copyright 2004-2019 Cortus S.A.S.
 *                        ALL RIGHTS RESERVED
 *
 * The entire notice above must be reproduced on all authorized copies
 * and any such reproduction must be pursuant to a licensing agreement 
 * from Cortus S.A.S. (http://www.cortus.com)
 *
 * $CortusRelease$
 * $FileName$
 *
 *********************************************************************************/
#ifndef PERIPH_CPU_H
#error "Do not include this bsp header directly. Please include periph_cpu.h"
#endif

#ifndef _TIMERCMP_H
#define _TIMERCMP_H
#include <machine/sfradr.h>

typedef struct Timer_Cmp
{
    /* compare value */
    volatile unsigned compare;

    /* Control register for PWM
       0 - set PWM when compare equals timer value, reset on external event
       1 - set PWM on external event, reset when compare equals timer value
       2 - set only when compare equals value - needs software reset
       3 - needs software set, reset only when compare equals value */
    volatile unsigned control;

    /* Set or reset PWM
       0 - none
       1 - resets the PWM
       2 - sets the PWM */
    volatile unsigned set_reset;
    
    /* Compare enable/disable */
    volatile unsigned enable;

    /* Interrupt status */
    volatile unsigned status;
    
    /* Interrupt mask/interrupt enable */
    volatile unsigned mask;
} Timer_Cmp;

#define timer1_cmpa ((Timer_Cmp *)SFRADR_TIMER1_CMPA)
#define timer1_cmpb ((Timer_Cmp *)SFRADR_TIMER1_CMPB)
#define timer2_cmpa ((Timer_Cmp *)SFRADR_TIMER2_CMPA)
#define timer2_cmpb ((Timer_Cmp *)SFRADR_TIMER2_CMPB)

#endif
