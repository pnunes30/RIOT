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

#ifndef _ANALOG_CTL_H
#define _ANALOG_CTL_H
#include <machine/sfradr.h>

typedef struct ANALOG_CTL
{
    /* LDO1 */

    /* LDO1 low power mode (if "1") otherwise full power mode */
    volatile unsigned ldo1_lp;
    /* LDO1 output current set up if ldo1_lp = "0" */
    volatile unsigned ldo1_iload;
    /* LDO1 output voltage trimming */
    volatile unsigned ldo1_trim;
    
    /* LDO2 */
    
    /* LDO2 enable */
    volatile unsigned dig_ldo2_en;
    /* LDO2 output current set up */
    volatile unsigned dig_ldo2_iload;
    /* LDO2 output voltage trimming */
    volatile unsigned dig_ldo2_trim;
    
    /* LDO2 enable */
    volatile unsigned pll_dig_ldo2_en;
    volatile unsigned pll_dig_ldo2_ovr_en;
    /* LDO2 output current set up */
    volatile unsigned pll_dig_ldo2_iload;
    /* LDO2 output voltage trimming */
    volatile unsigned pll_dig_ldo2_trim;

    /* LDO2 enable */
    volatile unsigned pll_pfdcp_ldo2_en;
    volatile unsigned pll_pfdcp_ldo2_ovr_en;
    /* LDO2 output current set up */
    volatile unsigned pll_pfdcp_ldo2_iload;
    /* LDO2 output voltage trimming */
    volatile unsigned pll_pfdcp_ldo2_trim;

    /* LDO2 enable */
    volatile unsigned pll_vco_ldo2_en;
    volatile unsigned pll_vco_ldo2_ovr_en;
    /* LDO2 output current set up */
    volatile unsigned pll_vco_ldo2_iload;
    /* LDO2 output voltage trimming */
    volatile unsigned pll_vco_ldo2_trim;
    
    /* LDO2 enable */
    volatile unsigned rxif_ldo2_en;
    volatile unsigned rxif_ldo2_ovr_en;
    /* LDO2 output current set up */
    volatile unsigned rxif_ldo2_iload;
    /* LDO2 output voltage trimming */
    volatile unsigned rxif_ldo2_trim;

    /* LDO2 enable */
    volatile unsigned rxtx_ldo2_en;
    volatile unsigned rxtx_ldo2_ovr_en;
    /* LDO2 output current set up */
    volatile unsigned rxtx_ldo2_iload;
    /* LDO2 output voltage trimming */
    volatile unsigned rxtx_ldo2_trim;
    
    /* LDO3 */
    
    /* LDO3 enable */
    volatile unsigned ldo3_en;
    /* LDO3 output current set up */
    volatile unsigned ldo3_iload;
    /* LDO3 output voltage trimming */
    volatile unsigned ldo3_trim;
    /* LDO3 bypass mode */
    volatile unsigned ldo3_bypass;

    /* LDO4 */

    /* LDO4 enable */
    volatile unsigned ldo4_en;
    /* LDO4 output current set up */
    volatile unsigned ldo4_iload;
    /* LDO4 output voltage trimming*/
    volatile unsigned ldo4_trim;
    
    /* DC-DC converter */
    
    /* DC-DC enable*/
    volatile unsigned dc_dc_en;
    /* undervoltage-lockout circuit mode */
    volatile unsigned dc_dc_uvlo_md;
    /* power frequency modulation mode */
    volatile unsigned dc_dc_pfm_md;
    /* operation frequency adjustment if pfm_md = "0" */
    volatile unsigned dc_dc_fosc;
    /* oscillator mode */
    volatile unsigned dc_dc_fosc_md;
    /* output current limit adjustment */
    volatile unsigned dc_dc_iadj;
    /* output voltage adjustment */
    volatile unsigned dc_dc_vadj;
    /* test mode */
    volatile unsigned dc_dc_st;
    /* Override enable */
    volatile unsigned dc_dc_ovr_en;

    /* 32 MHz crystal oscillator */

    volatile unsigned xtal_clk_adet_en; 
    volatile unsigned xtal_clk_buffer_en;
    volatile unsigned xtal_clk_c1_bank;
    volatile unsigned xtal_clk_c2_bank; 
    volatile unsigned xtal_clk_core_en; 
    volatile unsigned xtal_clk_en;
    volatile unsigned xtal_clk_itrim;

    /* 32 kHz crystal oscillator */
    
    /* crystal oscillator enable */
    volatile unsigned osc_en;
    /* current consumption adjustment */
    volatile unsigned xtal_lp_clk_cc;

    /* RF Bandgap */
    
    volatile unsigned bgref_en;            
    volatile unsigned bgref_ext_iref_en;
    volatile unsigned bgref_int_iref_en;
    volatile unsigned bgref_panic_ext_en;
    volatile unsigned bgref_panic_int_en;
    volatile unsigned bgref_reg_ext_en;
    volatile unsigned bgref_reg_int_en;
    
    /* This activates the 0.65volt reference for ALL the LDOs apart from the
       LDO always on before activating the LDOs this signal must be active */
    volatile unsigned ldo_vref_en;

    /* TBD: To Be Decided */
    volatile unsigned tbd_o;
    volatile unsigned tbd_i;
} ANALOG_CTL;

#define analog_ctl ((ANALOG_CTL *)SFRADR_ANALOG_CTL)

#endif
