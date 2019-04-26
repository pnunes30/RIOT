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

#ifndef _PW_MNG_SW_H
#define _PW_MNG_SW_H
#include <machine/sfradr.h>

typedef struct PW_MNG_SW
{
    /* Disable RF RX */
    volatile unsigned disable_rf_rx;
    /* Disable RF RX */
    volatile unsigned disable_rf_tx;
} PW_MNG_SW;

#define pw_mng_sw ((PW_MNG_SW *)SFRADR_PW_MNG_SW)

#endif
