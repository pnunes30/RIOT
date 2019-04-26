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

#ifndef _AES_H
#define _AES_H
#include <machine/sfradr.h>

typedef struct AES
{
    volatile unsigned ciphermode; // Encrypt mode = 0 (default)  Decrypt mode = 1
    volatile unsigned key;
    volatile unsigned data_in;
    volatile unsigned data_out;
    volatile unsigned key_en;
    volatile unsigned data_in_en; // start ??
    volatile unsigned mask;
    volatile unsigned status; //AES Status  0:Encryption/Decryption Data valid  1 if TextOut is valid */
} AES;

#ifdef __APS__
#define aes ((AES *)SFRADR_AES)
#else
extern AES __aes;
#define aes (&__aes)
#endif
#endif
