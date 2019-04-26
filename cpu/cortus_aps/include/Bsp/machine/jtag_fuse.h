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

#ifndef _JTAG_FUSE_H
#define _JTAG_FUSE_H
#include <machine/sfradr.h>

typedef struct JTAG_FUSE
{
    volatile unsigned jtag_fuse_check_ready;
} JTAG_FUSE;

#define jtag_fuse ((JTAG_FUSE *)SFRADR_JTAG_FUSE)

#endif
