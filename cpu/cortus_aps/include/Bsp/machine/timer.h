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

#ifndef _TIMER_H
#define _TIMER_H
#include <machine/sfradr.h>

typedef struct Timer
{
    /* Limit value */
    volatile unsigned period;

    /* Clock divider value
       0 - divide per 1
       1 - divide per 2
       2 - divide per 4
       3 - divide per 8 */
    volatile unsigned prescaler;

    /* Clock selection 
       0 - clock 0
       1 - clock 1
       2 - clock 2
       3 - clock 3 */
    volatile unsigned tclk_sel;

    /* Is the timer enabled? */
    volatile unsigned enable;

    /* Has the timer expired since last time this was reset? */
    volatile unsigned status;
    
    /* Interrupt mask/interrupt enable */
    volatile unsigned mask;
} Timer;

#define timer1 ((Timer *)SFRADR_TIMER1)
#define timer2 ((Timer *)SFRADR_TIMER2)
#endif
