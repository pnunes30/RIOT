/*
 * Copyright (C) 2017 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     boards_aps
 * @{
 *
 * @file
 * @brief       CORTUS peripheral configuration
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 */

#ifndef PERIPH_CONF_H
#define PERIPH_CONF_H

#include "periph_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Clock configuration
 * @{
 */
/* the main clock should be fixed to 32MHZ */
#define CLOCK_CORECLOCK     CLOCK_FREQUENCY     /* 12.5MHz, should be 32MHz */

/* peripheral clock setup */
/* see sfradr.h */
/** @} */

/**
 * @brief   Get the peripheral clock frequency for the selected source
 *
 * @param[in] source    selected source (four possible clock sources possible)
 *
 * @return              clock frequency in Hz
 */
static inline uint32_t clock_source(uint8_t source)
{
    switch(source)
    {
    case CLOCK_INPUT0:
        return CLOCK_SOURCE0;
    case CLOCK_INPUT1:
        return CLOCK_SOURCE1;
    case CLOCK_INPUT2:
        return CLOCK_SOURCE2;
    case CLOCK_INPUT3:
    default:
        return CLOCK_SOURCE3;
    }
}

/**
 * @brief   Get the timer prescaler value from the prescaler idx
 *
 * @param[in] prescaler idx as set in timer prescaler sfr.
 *
 * @return              prescaler value, iow clock freq divider
 */
#define NUM_PRESCALER_IDX 4

static inline uint8_t timer_prescaler_value(uint8_t idx)
{
    aps_assert (idx < NUM_PRESCALER_IDX);
    return (1 << idx);
}

///////////////////////////////TIMER////////////////////////////////////////////
/**
 * @name Xtimer configuration & Timer Peripheral config
 * @{
 */
/***************************** GLOBAL FLAGS ***********************************/
/* defined in cpu.h */
/*****************************************************************************/

#define _XTIMER_FIX_
#ifdef  _XTIMER_FIX_
#define _XTIMER_FIX_1_OVER_ARM_ /* fix race condition: overflow armed time avoids wrong list add when overflow is pending behind a set, ie in periodic_wakeup */
#define _XTIMER_FIX_2_RACE_SET_ /* fix race condition: IRQ between _xtimer_now and _xtimer_set_absolute in _xtimer_set */
#define _XTIMER_FIX_3_RACE_ABS_ /* fix race condition: IRQ between _xtimer_now and _lltimer_set in _xtimer_set_absolute */
#define _XTIMER_FIX_4_RACE_BCK_ /* BUG? backoff may cause overflow miss-detection */
//#define _XTIMER_FIX_5_RACE_REM_ /* BUG? remove  may cause overflow miss-detection */
#define _XTIMER_FIX_6_TIMELEFT_ /* fix race condition and wrong time_left computation when reference is after target */
#define _XTIMER_FIX_7_RACE_PER_ /* fix race condition: IRQ between _xtimer_now and _xtimer_set_absolute in _xtimer_periodic_wakeup */
#define _XTIMER_FIX_8_MASK_BUG_ /* fix mask bug for next_target in xtimer_callback leading to wrong isr_backoff comparison and llset in past */
#define _XTIMER_FIX_9_MASK_NOW_ /* fix race condition introduced by FIX6&2: _xtimer_now cannot be called with irq disabled when width<32*/
#define _XTIMER_FIX_10_CONV_BUG_ /* fix missing usec64 to ticks64 conversion in mutex_lock used by sem_timedwait */
#define _XTIMER_FIX_11_CONV_BUG_ /* fix missing usec64 to ticks64 conversion in evtimer */
#define _XTIMER_FIX_12_NEST_BUG_ /* fix nested interrupts bug in xtimer_mutex_timeout */
#endif /*_XTIMER_FIX_*/


#undef APS_TIMER_OVERFLOW0
#undef APS_XTIMER_OVERCLOCK
#undef APS_TIMER_CALIBRATE     /* should be defined if APS_DEBUG || APS_XTIMER_DEBUG, unless you tweak manually XTIMER_OVERHEAD */

/******************** beg of manual config ************************/
//#define ARCH_TIMER_MULT           (1)         // ARCH_TIMER_MULT!=1 is not implemented.

#define ARCH_XTIMER_CLK         (2)             // 32, 16, 8, 4 MHz
#define ARCH_XTIMER_PRE         (0)             // f/1 f/2 f/4 f/8
#define ARCH_XTIMER_HZ          (8000000UL)    // (1M * 32, 16, 8, 4, 2, 1, 0.5)
#undef ARCH_XTIMER_CONVERSION                  // must be defined if XTIMER_HZ is not supported (i.e isn't a power of 2 multiple of 1MHz).

#ifndef APS_XTIMER_OVERCLOCK
#define XTIMER_WIDTH            (TIMER_WIDTH)   // 32 unless overflow torture or ARCH_TIMER_MULT!=1
#define XTIMER_HZ               (ARCH_XTIMER_HZ)
#else
#define XTIMER_WIDTH            (26UL)          // 32 unless overflow torture or ARCH_TIMER_MULT!=1
#define XTIMER_HZ               (1000000UL)     /*(ARCH_TIMER_HZ)*/
#endif

#define ARCH_XTIMER_MAX         ((uint32_t)(0xffffffff>>(32-XTIMER_WIDTH))) /* the internal timer period value based on timer_width and internal mult */

/******************** timers config (not only xtimer specific) *******/
static const timer_conf_t timer_config[] = {
    {
#define TIMER_0_EN           (1)
        .dev      = (Timer *)SFRADR_TIMER1,
        .clk      = ARCH_XTIMER_CLK,
        .pre      = ARCH_XTIMER_PRE,
        .max      = ARCH_XTIMER_MAX,
#ifdef APS_TIMER_OVERFLOW0
		.irqo     = IRQ_TIMER1,					/* overflow isr enabled for xtimer debug */
#else
		.irqo     = 0, 							/* overflow isr disabled for xtimer */
#endif
        .cmp      = { (Timer_Cmp*)SFRADR_TIMER1_CMPA,
                      (Timer_Cmp*)SFRADR_TIMER1_CMPB },
        .irqn     = { IRQ_TIMER1_CMPA,
                      IRQ_TIMER1_CMPB },
        .cap      = (Timer_Cap*)SFRADR_TIMER1_CAPA,
    },
#ifdef SFRADR_TIMER2
    {
#define TIMER_1_EN           (1)
        .dev      = (Timer *)SFRADR_TIMER2,
        .clk      = 2,							/* 8MHz must match RTT_FREQUENCY if RTT used */
        .pre      = 0,
        .max      = TIMER_MAX_VAL,
		.irqo     = IRQ_TIMER2,					/* overflow isr enabled for RTT */
        .cmp      = { (Timer_Cmp*)SFRADR_TIMER2_CMPA,
                      (Timer_Cmp*)SFRADR_TIMER2_CMPB },
        .irqn     = { IRQ_TIMER2_CMPA,
                      0/*IRQ_TIMER2_CMPB*/ },	/* 0 means isr cmp disabled */
        .cap      = (Timer_Cap*)SFRADR_TIMER2_CAPA,
    }
#else
#ifdef USE_PERIPH_CORTUS_RTT
#error "RTT uses TIMER2"
#endif
#endif /*SFRADR_TIMER2*/
};
#define TIMER_NUMOF         (sizeof(timer_config) / sizeof(timer_config[0]))

/******************** end of manual config **************************/

/* Xtimer constants:
 *  Formula based on read_overhead~=32inst*(Nticks/inst), isr_overhead~=64inst*(Nticks/inst)
 *    OVERHEAD ~= 1*read(offset+now) + 1*read(now@set) + 1*set() + 1*isr_overhead + dbg
 *    BACKOFF  > |-OVERHEAD| + 1*read(if<now) + OVERHEAD + dbg
 *  Constraint: BACKOFF>2*OVERHEAD else timer is set in the past.
 *  Constants must pass tests/xtimer_usleep_short to be valid.
 */
#define _CAL_XTIMER_OVERHEAD(read_ov,isr_ov)    ((unsigned int)(2+3*read_ov+isr_ov))                        //xticks. used in set_absolute(timeout-OVERHEAD) => don't set too high to avoid overwarp.
#define _CAL_XTIMER_BACKOFF(read_ov,isr_ov)     ((unsigned int)(3*(_CAL_XTIMER_OVERHEAD(read_ov,isr_ov))))  //xticks. used to spin if (offset<BACKOFF). => don't set too low.
#define _CAL_XTIMER_ISR_BACKOFF(read_ov,isr_ov) ((unsigned int)(4*(_CAL_XTIMER_OVERHEAD(read_ov,isr_ov))))  //xticks. used to spin if (next_target<now+ISR_BACKOFF) => don't set too low

#ifndef APS_TIMER_CALIBRATE
#define APS_TIMER_READ_OVERHEAD     ((ARCH_XTIMER_HZ<<5)/CLOCK_FREQUENCY)
#define APS_TIMER_ISR_OVERHEAD      ((ARCH_XTIMER_HZ<<6)/CLOCK_FREQUENCY)
#define XTIMER_OVERHEAD             _CAL_XTIMER_OVERHEAD(APS_TIMER_READ_OVERHEAD, APS_TIMER_ISR_OVERHEAD)
#define XTIMER_BACKOFF              _CAL_XTIMER_BACKOFF(APS_TIMER_READ_OVERHEAD, APS_TIMER_ISR_OVERHEAD)
#define XTIMER_ISR_BACKOFF          _CAL_XTIMER_ISR_BACKOFF(APS_TIMER_READ_OVERHEAD, APS_TIMER_ISR_OVERHEAD)
#else
extern unsigned int aps_xtimer_overhead;
extern unsigned int aps_xtimer_backoff;
extern unsigned int aps_xtimer_isr_backoff;
#define XTIMER_OVERHEAD             (aps_xtimer_overhead)
#define XTIMER_BACKOFF              (aps_xtimer_backoff)
#define XTIMER_ISR_BACKOFF          (aps_xtimer_isr_backoff)
#endif
#define XTIMER_PERIODIC_SPIN    (XTIMER_BACKOFF*2)

#ifdef APS_XTIMER_OVERCLOCK
#define XTIMER_OVERCLOCK_RATIO  ((uint32_t)(ARCH_XTIMER_HZ/XTIMER_HZ))  /* for printf downscaling */
#else
#define XTIMER_OVERCLOCK_RATIO  (1)
#endif

#if (XTIMER_HZ % 1000000)
#define ARCH_XTICKS_PER_USEC    (XTIMER_HZ/1000000.0)   /* float xtick per micro second */
#else
#define ARCH_XTICKS_PER_USEC    (XTIMER_HZ/1000000UL)   /* uint  xtick per micro second */
#endif

#define ARCH_XTIMER_MAX_USEC32  ((uint32_t)(0xffffffff/ARCH_XTICKS_PER_USEC))

#ifdef APS_XTIMER_DEBUG
#ifdef XTIMER_DEV                               /* XTIMER_DEV must not be defined here, but must be known here */
#define ARCH_XTIMER_DEV         (XTIMER_DEV)
#else
#define ARCH_XTIMER_DEV         (0)             /* TIMER1/timer_config[0] assigned to xtimer */
#endif
#endif

#ifdef ARCH_XTIMER_CONVERSION
/* BE WARNED that xtimer implementation is loose on integer operation overflow.
 *
 * i.e. xtimer_set_msg(offset) works only with u32 offsets that are converted
 * internally from us to ticks on u32.
 * => If HZ>HZ_BASE it leads to undefined behaviour when offset>U32_MAX*HZ/HZ_BASE.
 */

/* arch specific xtimer tick_conversion hooks */
static inline uint32_t arch_xtimer_ticks_from_usec(uint32_t usec) {
    /* ***NOTE on overflows***
     * no overflow check is done in tick_conversion.h
     * i.e. when XTIMER_SHIFT=1 for XTIMER_HZ=2MHz we just multiply by 2.
     * Hence xtimer_usleep and in general all xtimer api must be called
     * carefully. They're ment to be called internally.
     * Consequence: to benefit from type checking protection
     * and avoid overflow as much as possible, the XTIMER_HZ should be
     * as close as possible from XTIMER_HZ_BASE.
     * But on APS this means less precise and longer response time from
     * the timer hardware...
     *
     * We keep the assert just for debug, but this should be
     * kept in production...
     */
    aps_assert (usec<=(0xffffffff/ARCH_XTICKS_PER_USEC));
    return (usec*ARCH_XTICKS_PER_USEC);  //FIXME: overflow check ???
}

static inline uint64_t arch_xtimer_ticks_from_usec64(uint64_t usec) {
    aps_assert (usec<=(0xffffffffffffffff/ARCH_XTICKS_PER_USEC));
    return (usec*ARCH_XTICKS_PER_USEC);  //FIXME: overflow check ???
}

static inline uint32_t arch_xtimer_usec_from_ticks(uint32_t ticks) {
    return (ticks/ARCH_XTICKS_PER_USEC); //FIXME: floor. round, ceil ???
}

static inline uint64_t arch_xtimer_usec_from_ticks64(uint64_t ticks) {
    return (ticks/ARCH_XTICKS_PER_USEC); //FIXME: floor. round, ceil ???
}
#endif /*ARCH_XTIMER_CONVERSION*/

#ifdef _XTIMER_FIX_9_MASK_NOW_
uint32_t timer_get_isr_tick(unsigned int tim);
#endif

#ifdef _XTIMER_FIX_6_TIMELEFT_
/*
 * Compute the signed 32bits time_left (second-first)
 * between two consecutive N-adic timestamps.
 * Warning: the formula is valid only if the distance
 * between first and second is less than half a N-cycle.
 */
#define int32_time_left_mask(first, second, N) \
    ( ((int32_t)((int32_t)(((uint32_t)second)<<(32-N)) - (int32_t)(((uint32_t)first)<<(32-N))))>>(32-N) )
#endif

#if defined(_XTIMER_FIX_1_OVER_ARM_) && defined(APS_XTIMER_DEBUG)
extern volatile uint32_t _xtimer_period_cnt;   /* exported from xtimer_core for overflow check */
#endif

#ifdef APS_TIMER_OVERFLOW0
extern volatile uint32_t   aps_timer_overflow_cnt;
#endif

/** @} */


///////////////////////////// RTT //////////////////////////////////////////////
/**
 * @name    RTT configuration
 * @{
 */
#ifdef USE_PERIPH_CORTUS_RTT
#ifndef SFRADR_TIMER2
#error "RTT uses TIMER2"
#else
#define RTT_NUMOF           (1U)
#define RTT_DEV             (1) 		//index of (Timer) used in timer_config[]
#define RTT_CHAN            (0) 		//RTT uses only the first compare block
#define RTT_MAX_VALUE       (timer_config[RTT_DEV].max)
#define RTT_FREQUENCY       (8000000U)  /* must match timer_config[RTT_DEV]*/
#endif
#endif /*USE_PERIPH_CORTUS_RTT*/
/** @} */


//////////////////////////////UART//////////////////////////////////////////////
/**
 * @name UART configuration
 * @{
 */

/* UART transmission mode: poll (blocking) and/or isr (jam). */
#define APS_UART_TX_WAIT_POLL_DEFAULT 200		/* poll mode: infinite loop at boot is not what we want in prod */
#if defined(MODULE_ETHOS)
#define APS_UART_TX_WAIT_POLL   	   -1       /* poll mode - 0:no, -1:forever(default), >0 num_poll_loops */
#define APS_UART_TX_WAIT_ISR   		    0       /* isr mode  - 0:no, -1:forever, >0 usec timeout  */
#define APS_UART_RX_ALWAYS_ON  		    1       /* rx mode   - 0:prevent console flood, 1:MUST for network itf */
#else
#define APS_UART_TX_WAIT_POLL   	   -1 //0   /* poll mode - 0:no, -1:forever(default), >0 num_poll_loops */
#define APS_UART_TX_WAIT_ISR   		    0 //-1  /* isr mode  - 0:no, -1:forever, >0 usec timeout  */
#if defined(MODULE_NEWLIB_SYSCALLS_DEFAULT)
/* syscalls redefinitions and stdio_read assumes rx always on */
#define APS_UART_RX_ALWAYS_ON  		    1       /* rx mode   - 0:prevent console flood, 1:MUST for network itf */
#else
#define APS_UART_RX_ALWAYS_ON  		    0       /* rx mode   - 0:prevent console flood, 1:MUST for network itf */
#endif
#endif

#if (APS_UART_TX_WAIT_ISR != 0)
#warning "WARNING-TBI: TX ISR may cause priority inversion or deadlocks problems => should be DISABLED for now. -jm-"
#endif

static const uart_conf_t uart_config[] = {
		{
#define UART_0_EN           (1)
            .dev           = (Uart *)SFRADR_UART1,
            .rx_irqn       = IRQ_UART1_RX,
            .rx_always_on  = 1, //APS_UART_RX_ALWAYS_ON,
            .tx_irqn       = IRQ_UART1_TX,
            .tx_wait_poll  = APS_UART_TX_WAIT_POLL,
            .tx_wait_isr   = APS_UART_TX_WAIT_ISR,
        },
#ifdef SFRADR_UART2
        {
#define UART_1_EN           (1)
            .dev           = (Uart *)SFRADR_UART2,
            .rx_irqn       = IRQ_UART2_RX,
            .rx_always_on  = 1,						/* always on for network (slip) */
            .tx_irqn       = IRQ_UART2_TX,
            .tx_wait_poll  = APS_UART_TX_WAIT_POLL,
            .tx_wait_isr   = APS_UART_TX_WAIT_ISR,
        }
#endif
};
#define UART_NUMOF         (sizeof(uart_config) / sizeof(uart_config[0]))

/* UART pin configuration: NA */

/* UART extra C declarations */
#ifndef HAVE_UART_T
#define HAVE_UART_T
typedef unsigned int uart_t;
#endif

#ifndef HAVE_UART_ISR_CTX_T
#define HAVE_UART_ISR_CTX_T
typedef void(*uart_tx_cb_t)(void *arg, uart_t uart);
typedef void(*uart_rx_cb_t)(void *arg, uint8_t data);
typedef struct {
    uart_rx_cb_t rx_cb;     /**< data received interrupt callback */
    uart_tx_cb_t tx_cb;     /**< tx ready interrupt callback */
    void *rx_arg;           /**< argument of rx_cb routine */
    void *tx_arg;           /**< argument of tx_cb routine */
    int32_t tx_wait_poll;   /**< dynamic reconfigurable copy of uart_conf_t (due to xtimer dep)*/
    int32_t tx_wait_isr;    /**< dynamic reconfigurable copy of uart_conf_t (due to xtimer dep)*/
} uart_isr_ctx_t;
#endif

#if (APS_UART_TX_WAIT_ISR != 0)         /* isr mode on */
# ifndef MODULE_XTIMER
# define MODULE_XTIMER 1
# endif
# ifdef MODULE_UART_STDIO
void uart_init_tx_isr(uart_t uart);     /* used to reconfigure tx_isr for stdio_uart which is initialized before xtimer */
# endif
#endif

#ifdef MODULE_UART_STDIO
void uart_start_rx_isr(uart_t uart);
void uart_stop_rx_isr(uart_t uart);
#endif

/** @} */

////////////////////////////// ADC /////////////////////////////////////////////
/**
 * @name   ADC configuration
 *
 * The configuration consists simply of a list of channels that should be used
 * @{
 */
#define ADC_NUMOF          (1U)
/** @} */


////////////////////////////// GPIO ////////////////////////////////////////////
#ifndef HAVE_GPIO_ISR_CTX_T
#define HAVE_GPIO_ISR_CTX_T
typedef void (*gpio_cb_t)(void *arg);
typedef struct {
    gpio_cb_t cb;           /**< interrupt callback */
    void *arg;              /**< optional argument */
    gpio_flank_t in_flank;		/**< flank config for edge and level interrupt input pins */
} gpio_isr_ctx_t;
#endif

////////////////////////////// LED /////////////////////////////////////////////
/**
 * @name    Led definition
 * @{
 */
#define LED_NUMOF          (8U)
/** @} */

////////////////////////////// BUTTON //////////////////////////////////////////
/**
 * @name    User button definitions
 * @{
 */
#define USERBUTTONS_NUMOF  2
#define BUTTON0            GPIO_PIN(gpioPortA, 4)
#define BUTTON1            GPIO_PIN(gpioPortA, 5)
/** @} */


////////////////////////////// SPI /////////////////////////////////////////////
 /**
 * @name    SPI configuration
 * @{
 */
#ifdef USE_PERIPH_CORTUS_SPI
/* spi_divtable must match the spi_clk_t enum overriden in periph_cpu.h */
#if (CLOCK_FREQUENCY == 50000000)
#define SPI_SELCLK 0
static const uint16_t spi_divtable[5] = {
        /* for clock input @ 50MHz */
        499,  /* -> 100 KHz */
        124,  /* -> 400 KHz */
        49,   /* -> 1 MHz */
        9,    /* -> 5 MHz */
        4     /* -> 10 MHz */
};
#elif (CLOCK_FREQUENCY == 16000000)
#define SPI_SELCLK 0	/* For CC1101, fsclk.max=6.5MHz. the sysclock and clk1 are @16MHz, clk0 is @32MHz */
static const uint16_t spi_divtable[5] = {
        /* for clock input @ 16MHz */
        320-1,  /* -> 100 KHz */
        80-1,   /* -> 400 KHz */
        32-1,   /* -> 1 MHz */
        6-1,    /* -> 5.333 MHz */
        3-1     /* -> 10.666 MHz */
};
#else
#error "no spi_divtable matching the given clock frequency"
#endif

#define SPI_CS_PIN 		GPIO_PIN(SFRADR_GPIO1, 2)

static const spi_conf_t spi_config[] = {
    {
        .dev   = (SPI *)SFRADR_SPI2,
        .cs_pin = SPI_CS_PIN,
    },
#ifdef USE_PERIPH_CORTUS_MTD_FPGA_M25P32
# ifndef SPI_CS_UNDEF_MAPPED_ON_CLK_EN
# define SPI_CS_UNDEF_MAPPED_ON_CLK_EN 1  /* hack necessary for m25p32/mtd_spi_nor where unmapped cs line must be explicitly pulled high after some transfer_bytes */
# endif
    {
        .dev   = (SPI *)SFRADR_SPI,
        .cs_pin = GPIO_UNDEF, /*SPI_CS_UNDEF*/
    }
#endif
};

#define SPI_NUMOF         (sizeof(spi_config) / sizeof(spi_config[0]))

#endif /*USE_PERIPH_CORTUS_SPI*/
/** @} */


/////////////////////////////CC110X/////////////////////////////////////////////
/**
 * @name    CC1101 PIN definitions
 * @{
 */
#define CC1101_SPI_USART    1  // not used
#define CC1101_SPI_BAUDRATE 8  // divider
#define CC1101_SPI_BUS		0	//index in spi_config[]
#define CC1101_SPI_PIN_CS   SPI_CS_PIN
#define CC1101_GDO0_PIN     GPIO_PIN(SFRADR_GPIO_EDGE1, 0)
#define CC1101_GDO2_PIN     GPIO_PIN(SFRADR_GPIO_EDGE1, 1)
#define CC1101_GDO1_PIN		GPIO_PIN(SFRADR_GPIO_EDGE2, 0)	//FIXME: fake input (should be SO). must be always low.
/** @} */


////////////////////////////////////////////////////////////////////////////////
/**
 * @name    Sx127x PIN definitions
 * @{
 */
#define SX127x_SPI_INDEX    1
#define SX127x_SPI_BAUDRATE 8 //10000000
#define SX127x_SPI_BUS      0	//index in spi_config[]
#define SX127x_SPI_PIN_CS   SPI_CS_PIN
#define SX127x_DIO0_PIN     GPIO_PIN(SFRADR_GPIO_EDGE1, 0)
#define SX127x_DIO1_PIN     GPIO_PIN(SFRADR_GPIO_EDGE1, 1)
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CONF_H */
