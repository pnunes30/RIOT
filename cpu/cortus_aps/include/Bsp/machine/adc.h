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

#ifndef _ADC_H
#define _ADC_H
#include <machine/sfradr.h>

typedef struct ADC
{
    volatile unsigned enable;     /**< ADC enable Register */
    volatile unsigned input_en;
    volatile unsigned conv_exe;   /**< Conversion start Register */
    volatile unsigned conv_md;    /**< Conversion mode Register  0 single conversion / 1 continuous;*/
    volatile unsigned comp_md;    /**< Comparator current adjustement Register ??? */
    volatile unsigned buf_en;
    volatile unsigned buf_md;
    volatile unsigned buf1_cc;
    volatile unsigned buf1_out;
    volatile unsigned buf2_cc;
    volatile unsigned buf2_pm;
    volatile unsigned clk_div;    /**< Clock divider Register */
    volatile unsigned mask;       /**< Interrupt Enable Register */
    volatile unsigned data;       /**< Conversion Result Data Register */
    volatile unsigned status;     /**< Status Register */
} ADC;

#ifdef __APS__
#define adc ((ADC *)SFRADR_ADC)
#else
extern ADC __adc;
#define adc (&__adc)
#endif
#endif
