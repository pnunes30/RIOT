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

#ifndef _PW_MNG_AON_H
#define _PW_MNG_AON_H
#include <machine/sfradr.h>

typedef struct PW_MNG_AON
{
    /* Disable power */
    volatile unsigned disable_pwr;

    /* Time between wakeup and restore_power */
    volatile unsigned delay;

    /* Disable power switch1 */
    volatile unsigned disable_pwr_switch1;

    /* Safe mode */
    volatile unsigned safe_mode;

} PW_MNG_AON;

#define pw_mng_aon ((PW_MNG_AON *)SFRADR_PW_MNG_AON)

#endif
