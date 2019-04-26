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

#ifndef _CLK_RST_MNG_H
#define _CLK_RST_MNG_H
#include <machine/sfradr.h>

typedef struct CLK_RST_MNG
{
    /* Input clock selection */
    volatile unsigned clk_sel;
} CLK_RST_MNG;

#define clk_rst_mng ((CLK_RST_MNG *)SFRADR_CLK_RST_MNG)

#endif
