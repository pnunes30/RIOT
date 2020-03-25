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

#ifndef _BSTATE_H
#define _BSTATE_H
#include <machine/sfradr.h>

typedef struct BSTATE
{
    volatile unsigned bsel;
    volatile unsigned trap_base_ad;
} BSTATE;

#define bstate ((BSTATE *)SFRADR_BSTATE)

#endif
