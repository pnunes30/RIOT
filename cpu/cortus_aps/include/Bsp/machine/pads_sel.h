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

#ifndef _PADS_SEL_H
#define _PADS_SEL_H
#include <machine/sfradr.h>

typedef struct PADS_SEL
{
    volatile unsigned pad0_sel;
    volatile unsigned pad0_config;
    volatile unsigned pad1_sel;
    volatile unsigned pad1_config;
    volatile unsigned pad2_sel;
    volatile unsigned pad2_config;
    volatile unsigned pad3_sel;
    volatile unsigned pad3_config;
    volatile unsigned pad4_sel;
    volatile unsigned pad4_config;
    volatile unsigned pad5_sel;
    volatile unsigned pad5_config;
    volatile unsigned pad6_sel;
    volatile unsigned pad6_config;
    volatile unsigned pad7_sel;
    volatile unsigned pad7_config;
    volatile unsigned pad8_sel;
    volatile unsigned pad8_config;
    volatile unsigned pad9_sel;
    volatile unsigned pad9_config;
    volatile unsigned pad10_sel;
    volatile unsigned pad10_config;
    volatile unsigned pad11_sel;
    volatile unsigned pad11_config;
} PADS_SEL;

#define pads_sel ((PADS_SEL *)SFRADR_PADS_SEL)

#endif
