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

#ifndef _GPIO_H
#define _GPIO_H
#include <machine/sfradr.h>

typedef struct Gpio
{
    volatile unsigned out;
    volatile unsigned in;
    volatile unsigned dir;
    volatile unsigned old_in;
    volatile unsigned mask;
    volatile unsigned level_sel;
    volatile unsigned rs_edge_sel;
    volatile unsigned fl_edge_sel;
    volatile unsigned edge;
} Gpio;

/** GPIO ports identificator. */
typedef enum
{
  gpioPortA = SFRADR_GPIO1, /**< Port A */
  gpioPortB = SFRADR_GPIO2, /**< Port B */
} GPIO_Port_TypeDef;

/** Pin mode. For more details on each mode, please refer to the APS peripheral reference manual. */

#define HAVE_GPIO_MODE_T
typedef enum
{
  /** Input  without pull up*/
  gpioModeInput = 0,
  /** Input  with pull up*/
  gpioModeInputPull = 0xFF,
  /** output  with pull up*/
  gpioModePushPull = 1,
} GPIO_Mode_TypeDef;

#ifdef __APS__
#define gpio1 ((Gpio *)SFRADR_GPIO1)
#define gpio2 ((Gpio *)SFRADR_GPIO2)
#else
extern Gpio __gpio1;
extern Gpio __gpio2;
#define gpio1 (&__gpio1)
#define gpio2 (&__gpio2)
#endif
#endif
