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

#ifndef _FLASH_CTL_H
#define _FLASH_CTL_H
#include <machine/sfradr.h>

typedef struct FLASH_CTL
{
    // ctrreg[17] write enable
    // ctrreg[16] !LCK_CFG (be aware that this bit is inverted, for compatibility reason with previous SW)
    
    // ctrreg[15] LVCTL
    // ctrreg[14] Flash test mode
    // ctrreg[13] VREAD1
    // ctrreg[12] VREAD0
    // ctrreg[11] RETRY[1]
    // ctrreg[10] RETRY[0]
    // ctrreg[9]  ARRDN[1]
    // ctrreg[8]  ARRDN[0]
    
    // ctrreg[7]  NVR
    // ctrreg[6]  DPD
    // ctrreg[5]  RECALL
    // ctrreg[4]  NVR_CFG
    // ctrreg[3]  configuration register write, activates CONFEN
    // ctrreg[2]  chip erase, activates ERASE and CHIP
    // ctrreg[1]  sector erase, activates ERASE
    // ctrreg[0]  slow/fast write, activates PREPG
    volatile unsigned ctrreg;
    volatile unsigned tACC;    
    volatile unsigned tADH;    
    volatile unsigned tADS;    
    volatile unsigned tNVS;    
    volatile unsigned tRCV1;    
    volatile unsigned tRCV2;    
    volatile unsigned tRCV3;    
    volatile unsigned tPGS;    
    volatile unsigned tPREPROG;    
    volatile unsigned tPREPGH;    
    volatile unsigned tPROG;    
    volatile unsigned tPGH;    
    volatile unsigned tERASE;    
    volatile unsigned tSCE;    
    volatile unsigned tRW;    
    volatile unsigned tCFS;    
    volatile unsigned tCFH;    
    volatile unsigned tCONFEN;    
    volatile unsigned tCFL;
    volatile unsigned tMS;
    volatile unsigned tmen;
    volatile unsigned tmval;
} FLASH_CTL;

#define FLASH_PREPG 1 << 0
#define FLASH_ERASE 1 << 1
#define FLASH_CHIP 1 << 2
#define FLASH_CONFEN 1 << 3
#define FLASH_NVR_CFG 1 << 4
#define FLASH_RECALL 1 << 5
#define FLASH_DPD 1 << 6
#define FLASH_NVR 1 << 7
#define FLASH_ARRDN_0 1 << 8
#define FLASH_ARRDN_1 1 << 9
#define FLASH_RETRY_0 1 << 10
#define FLASH_RETRY_1 1 << 11
#define FLASH_VREAD0 1 << 12
#define FLASH_VREAD1 1 << 13
#define FLASH_TEST 1 << 14
#define FLASH_LVCTL 1 << 15
#define FLASH_LCK_CFG_INV 1 << 16
#define FLASH_WR_EN 1 << 17

#define flash_ctl ((FLASH_CTL *)SFRADR_FLASH_CTL)

#endif
