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

#ifndef _FLASH_AD_ADDER_H
#define _FLASH_AD_ADDER_H
#include <machine/sfradr.h>

typedef struct FLASH_AD_ADDER
{
    volatile unsigned enable;
    volatile unsigned value;
} FLASH_AD_ADDER;

#define flash_ad_adder ((FLASH_AD_ADDER *)SFRADR_FLASH_AD_ADDER)

#endif
