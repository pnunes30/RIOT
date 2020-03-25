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
#ifndef PERIPH_CPU_H
#error "Do not include this bsp header directly. Please include periph_cpu.h"
#endif

#ifndef _SFRADR_H
#define _SFRADR_H

/* ID FPGA#rrjiba@201805291100 (ciot25eval1) : aps3r, dual uart (FT42), 32/16MHz */

#ifndef APS_CPU
#define APS_CPU				"aps3r"
#endif

#define CLOCK_FREQUENCY              32000000
#define USB2_CLOCK_FREQUENCY         50000000

#define CLOCK_SOURCE0               (32000000U)
#define CLOCK_SOURCE1               (16000000U)
#define CLOCK_SOURCE2               (8000000U)
#define CLOCK_SOURCE3               (4000000U)
#define NUM_CLOCK_SOURCE    4

#define SFRADR_IC                   0x40000000
#define SFRADR_IC1                  0x40000100

#define SFRADR_UART1                0x40001000
#define SFRADR_UART2                0x40001100

#define SFRADR_COUNTER1             0x40002000

#define SFRADR_TIMER1               0x40002200
#define SFRADR_TIMER1_CAPA          0x40002220
#define SFRADR_TIMER1_CMPA          0x40002300
#define SFRADR_TIMER1_CMPB          0x40002320
#define SFRADR_TIMER1_CMPC          0x40002340

#define SFRADR_TIMER2               0x40002400
#define SFRADR_TIMER2_CAPA          0x40002420
#define SFRADR_TIMER2_CMPA          0x40002500
#define SFRADR_TIMER2_CMPB          0x40002520
#define SFRADR_TIMER2_CMPC          0x40002540

#define SFRADR_TIMER3               0x40002600
#define SFRADR_TIMER3_CMPA          0x40002620
#define SFRADR_TIMER3_CMPB          0x40002640
#define SFRADR_TIMER3_CMPC          0x40002660

#define SFRADR_WDT                  0x40003000
#define SFRADR_WDT_WD               0x40003100

#define SFRADR_GPIO1                0x40004000
#define SFRADR_GPIO2                0x40004100

#define SFRADR_SPI                  0x40005000
#define SFRADR_SPI2                 0x40005100

#define SFRADR_I2C                  0x40006000

#define SFRADR_IFS                  0x40008000
#define SFRADR_BSTATE               0x40009000

#define SFRADR_FLASH_AD_ADDER       0x40009100

#define SFRADR_BRKPTS               0x50000000   /* IRQ 4 */
#define SFRADR_BRKPTS1              0x50000100

#define SFRADR_TRNG                 0x40050000

#define SFRADR_AES                  0x40060000

#define SFRADR_JTAG_FUSE            0x40070000

#define SFRADR_FLASH_CTL            0xa0000000

#define SFRADR_CLK_RST_MNG          0x40080000

#define SFRADR_PW_MNG_AON           0x40090000
#define SFRADR_PW_MNG_SW            0x4009a000

#define SFRADR_ADC                  0x400a0000

#define SFRADR_RTC_TIMER            0x400b0000
#define SFRADR_RTC_TIMER_CMPA       0x400b0300

#define SFRADR_PADS_SEL             0x400c0000

#define SFRADR_PVT                  0x400d0000

#define SFRADR_ANALOG_CTL           0x400e0000

#define SFRADR_XCVR                 0x70000000
#define SFRADR_CODEC                0x70000030
#define SFRADR_BASEBAND             0x70000058
#define SFRADR_RADIO                0x700000EC
#define SFRADR_DATA_IF              0x70010000
#define SFRADR_BYPASS_DATA_IF       0x70020000
#define SFRADR_CIC_IQ_DATA_IF       0x70030014

/* Memory map */
#define ADR_IMEM                    0x80000000
#define ADR_DMEM                    0x00000000
#define ADR_DMEM_AON                0x08000000

/* IRQd */

#define IRQ_DEBUGGER_STOP           4

#define IRQ_UART1_TX                5
#define IRQ_UART1_RX                6

#define IRQ_UART2_TX                7
#define IRQ_UART2_RX                8

#define IRQ_COUNTER1                9

#define IRQ_I2C_TX                  10
#define IRQ_I2C_RX                  11

#define IRQ_GPIO1                   12

#define IRQ_SPI_TX                  13
#define IRQ_SPI_RX                  14

#define IRQ_SPI2_TX                 15
#define IRQ_SPI2_RX                 16

#define IRQ_TIMER1                  17
#define IRQ_TIMER1_CAPA             18
#define IRQ_TIMER1_CMPA             19
#define IRQ_TIMER1_CMPB             33
#define IRQ_TIMER1_CMPC             34

#define IRQ_TIMER2                  20
#define IRQ_TIMER2_CAPA             21
#define IRQ_TIMER2_CMPA             22
#define IRQ_TIMER2_CMPB             35
#define IRQ_TIMER2_CMPC             36

#define IRQ_TIMER3                  37
#define IRQ_TIMER3_CMPA             38
#define IRQ_TIMER3_CMPB             39
#define IRQ_TIMER3_CMPC             40

#define IRQ_RTC_TIMER               23
#define IRQ_RTC_TIMER_CMPA          24

#define IRQ_GPIO2                   25

#define IRQ_XCVR_TX                 26
#define IRQ_XCVR_RX                 27

#define IRQ_DATA_IF_TX              28
#define IRQ_DATA_IF_RX              29

#define IRQ_BYPASS_DATA_IF_TX       41
#define IRQ_BYPASS_DATA_IF_RX       42
#define IRQ_CIC_IQ_DATA_IF_RX       43

#define IRQ_AES_CIPHER              30

#define IRQ_ADC                     31

#define IRQ_PVT                     32

#endif
