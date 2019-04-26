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

#ifndef _TRNG_H
#define _TRNG_H
#include <machine/sfradr.h>

typedef struct trng_struct
{
    /* TRNG Enable */
    volatile unsigned en;

    /* TRNG Value */
    volatile unsigned val;

} trng_struct;

#ifdef __APS__
#define trng ((trng_struct *)SFRADR_TRNG)
#endif


#endif
