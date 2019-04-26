/*
 * Copyright (C) 2018 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @{
 *
 * @file
 * @brief       Implementation of the CPU initialization
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @}
 */

#include <stdint.h>
#include "cpu.h"
#include "irq.h"

#include "periph_conf.h"
#include "periph/init.h"

#include "board.h"
#include "kernel_init.h"

#ifdef APS_DEBUG
aps_debug_t __aps_debug =   {
        .irq_31=0,
		.irq_cnt=0,
		.irq_tim_overflow=0,
		.irq_tim_cmp=0,
		.irq_uart1_rx=0,
		.irq_uart1_tx=0,
		.irq_uart2_rx=0,
		.irq_uart2_tx=0,
		.irq_nested=0,
        .ctx_save=0,
        .ctx_restore=0,
        .tim_last_cap_loop_cnt=0,
        .tim_max_cap_loop_cnt=0,
		.uart_max_loop_cnt=0,
		.uart_tx_timeout_cnt=0,
#ifdef APS_XTIMER_DEBUG
        .xtimcb.idx=APS_MAX_HIST,
        .timset.idx=APS_MAX_HIST,
        .tim_overflow_err_cnt=0,
        .tim_overflow_last_cmp=0,
        .tim_last_cap_value=0,
        .tim_last_cap_pid=0,
        .tim_xtimer_setab=0,
        .tim_xtimer_set32=0,
        .tim_xtimer_set64=0,
        .tim_xtimer_shoot=0,
        .tim_xtimer_list_ad0=0,
        .tim_xtimer_list_ad1=0,
        .tim_xtimer_list_adv=0,
        .tim_xtimer_list_cnt=0,
        .tim_xtimer_list_max=0,
#endif
};

/* used only to catch by breakpoint a failed assertion */
void __attribute__((noinline)) assert_break(void)
{
    int i=10;/*???loop to let openocd catch the breakpoint*/
    do { } while (i--);
}
/* used only to set a dummy breakpoint */
void __attribute__((used,noinline)) debug_break(int i)
{
    do { } while (i--);
}
#endif /*APS_DEBUG*/

#ifdef PROVIDES_PERIPH_INIT /*overload periph_common version*/
void periph_init(void)
{
#ifdef MODULE_PERIPH_GPIO
    /* initialize the boards LEDs */
    gpio_init(LED0_PIN, GPIO_OUT);
#endif

#ifdef MODULE_PERIPH_SPI
    /* initialize configured SPI devices */
    for (unsigned i = 0; i < SPI_NUMOF; i++) {
        spi_init(SPI_DEV(i));
    }
#endif

    /* Initialize RTC */
#ifdef MODULE_PERIPH_RTC
    rtc_init();
#endif

#ifdef MODULE_PERIPH_HWRNG
    hwrng_init();
#endif
}
#endif

static void cpu_clock_init(void)
{
    //TODO
}

/**
 * @brief   Initialize the CPU, set IRQ priorities
 */
void cpu_init(void)
{
    /* initialize the APS core */
    //aps_init();
    /* initialize the clock system */
    cpu_clock_init();
    /* trigger static peripheral initialization */
    periph_init();
}

// function called at the end of crt0::start() instead of main().
void aps_start(void)
{
    board_init();

    kernel_init(); /*no return*/
}
