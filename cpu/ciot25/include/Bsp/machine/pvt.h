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

#ifndef _PVT_H
#define _PVT_H
#include <machine/sfradr.h>


#define PVT_STATUS_READ_COMPLETE 0x01
#define PVT_STATUS_READ_IN_PROGRESS 0x02

typedef struct PVT
{
    volatile unsigned read;
    volatile unsigned mode;
    volatile unsigned su_sel;
    volatile unsigned pu_sel;
    volatile unsigned value;
    volatile unsigned status;
    volatile unsigned mask;
    volatile unsigned test_en;
    volatile unsigned test_sel;
    volatile unsigned enable;
} PVT;

#ifdef __APS__
#define pvt ((PVT *)SFRADR_PVT)
#else
extern PVT __pvt;
#define pvt (&__pvt)
#endif
#endif
