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

#ifndef _TIMERCAP_H
#define _TIMERCAP_H
#include <machine/sfradr.h>

typedef struct Timer_Cap
{
    /* capture input selection */
    volatile unsigned in_cap_sel;

    /* Capture edge selection
       0 - positive edge
       1 - negative edge
       2 - both edges
       3 - not valid */
    volatile unsigned edge_sel;

    /* Captured value */
    volatile unsigned value;
    
    /* Capture enabled */
    volatile unsigned enable;

    /* Interrupt status */
    volatile unsigned status;
    
    /* Interrupt mask/interrupt enable */
    volatile unsigned mask;

    /* Software capture (use with in_cap_sel = 4) */
    volatile unsigned capture;

} Timer_Cap;

#define timer1_capa ((Timer_Cap *)SFRADR_TIMER1_CAPA)
#define timer1_capb ((Timer_Cap *)SFRADR_TIMER1_CAPB)
#define timer1_capc ((Timer_Cap *)SFRADR_TIMER1_CAPC)
#define timer2_capa ((Timer_Cap *)SFRADR_TIMER2_CAPA)
#define timer2_capb ((Timer_Cap *)SFRADR_TIMER2_CAPB)
#define timer2_capc ((Timer_Cap *)SFRADR_TIMER2_CAPC)

#endif
