/*
 * Copyright (C) 2018 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @ingroup     drivers_periph_uart
 * @{
 *
 * @file
 * @brief       Low-level UART driver implementation
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */

#include "cpu.h"
#include "sched.h"
#include "thread.h"
#include "assert.h"
#include "periph_conf.h"
#include "periph/uart.h"

#include "mutex.h"

#ifdef USE_PERIPH_CORTUS_UART

/**
 * @brief   Allocate memory to store the callback functions
 */
static uart_isr_ctx_t uart_isr_ctx[UART_NUMOF];

/* Do not build tx isr components if not required. */
#if (APS_UART_TX_WAIT_ISR != 0)         /* isr mode on */
#include "xtimer.h"
static mutex_t _uart_tx_mutex[UART_NUMOF];
#define aps_uart_lock(uart)   mutex_lock(&_uart_tx_mutex[uart])
#define aps_uart_unlock(uart) mutex_unlock(&_uart_tx_mutex[uart])
static void _uart_tx_ready(void* arg, uart_t uart);
#define tx_wait_poll(uart)    uart_isr_ctx[uart].tx_wait_poll
#define tx_wait_isr(uart)     uart_isr_ctx[uart].tx_wait_isr
#else
#define aps_uart_lock(uart)
#define aps_uart_unlock(uart)
#define tx_wait_poll(uart)    APS_UART_TX_WAIT_POLL
#define tx_wait_isr(uart)     0
#endif

void uart_start_rx_isr(uart_t uart);
void uart_stop_rx_isr(uart_t uart);

/**
 * @brief   Get the timer device
 */
static inline Uart *dev(uart_t uart)
{
    return uart_config[uart].dev;
}

int uart_init(uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg)
{
    assert(uart < UART_NUMOF);

    /* save ISR context */
    uart_isr_ctx[uart].rx_cb  = rx_cb;
    uart_isr_ctx[uart].rx_arg = arg;

    /* No need to configure TX and RX pin */

    /* disable the clock */
    dev(uart)->enable = 0;

    /* reset UART configuration -> defaults to 8N1 mode */
    dev(uart)->config = 0; // 8 bits, no parity, 1 stop bit, flow control disabled

    /* calculate and apply baudrate */
    dev(uart)->selclk = CLOCK_INPUT0; // 0:50MHz, 1:25MHz, 2:12.5MHZ, 3:3.125MHz
    dev(uart)->divider = 8*(clock_source(CLOCK_INPUT0)/baudrate);

    /* enable the UART module */
    dev(uart)->enable = 1;

    /* enable RX interrupt if applicable */
    if (rx_cb) {
        aps_irq[uart_config[uart].rx_irqn].ien = 1;
        if (uart_config[uart].rx_always_on)
            uart_start_rx_isr(uart);
    }

#if (APS_UART_TX_WAIT_ISR != 0)         /* isr mode on */
    /* prepare TX isr mode. Do everything possible only once. */
    uart_init_tx_isr(uart);
#endif

    return UART_OK;
}

void uart_start_rx_isr(uart_t uart)
{
    //FIXME: not thread safe.
    dev(uart)->rx_mask = 1;  /* start Rx interrupt */
}

void uart_stop_rx_isr(uart_t uart)
{
    //FIXME: not thread safe.
    if (!uart_config[uart].rx_always_on)
        dev(uart)->rx_mask = 0;  /* stop Rx interrupt */
}

void uart_init_tx_isr(uart_t uart)
{
#if (APS_UART_TX_WAIT_ISR != 0)         /* isr mode on */
    irq[uart_config[uart].tx_irqn].ien = 1;
    mutex_init(&_uart_tx_mutex[uart]);
    uart_isr_ctx[uart].tx_cb  = _uart_tx_ready;
    if (timer_config[XTIMER_DEV].dev->enable != 0)
    {
        uart_isr_ctx[uart].tx_wait_poll = uart_config[uart].tx_wait_poll;
        uart_isr_ctx[uart].tx_wait_isr  = uart_config[uart].tx_wait_isr;
    }
    else
#endif
    {
        /* early prints use bounded poll to avoid boot blocking, and no isr as they can't rely on xtimer */
        uart_isr_ctx[uart].tx_wait_poll = APS_UART_TX_WAIT_POLL_DEFAULT;
        uart_isr_ctx[uart].tx_wait_isr  = 0;
    }
}

void uart_poweron(uart_t uart)
{
    volatile int dump = 0;
    (void) dump;

    assert(uart < UART_NUMOF);

    /* Flush the RX buffer. */
    while(dev(uart)->rx_status)
        dump = dev(uart)->rx_data;

    dev(uart)->rx_mask = 1;  /* start Rx interrupt */
}

void uart_poweroff(uart_t uart)
{
    assert(uart < UART_NUMOF);
    dev(uart)->rx_mask = 0; /* stop Rx interrupt */
    dev(uart)->tx_mask = 0; /* stop Tx interrupt */
}

static inline void _uart_outch_raw(uart_t uart, const uint8_t c)
{
    /* never block here. if not ready, the previous char is dropped. no remorses.*/
    dev(uart)->tx_data = c;
}

#if (APS_UART_TX_WAIT_ISR != 0)         /* isr mode on */
static void _uart_tx_ready(void* arg, uart_t uart)
{
    dev(uart)->tx_mask = 0;
    mutex_t * mutex = (mutex_t *)uart_isr_ctx[uart].tx_arg;
    if (mutex) mutex_unlock(mutex);
    /* thread_yield at end of isr => WAIT_UNBLOCKED */
}
static inline void _uart_tx_wait_isr(uart_t uart, uint32_t max_usec)
{
    /* only one waiter/transmitter at a time:
     *  so here we are already protected by the global mutex,
     *  and there's no risk of race on global variables.
     */
    mutex_t mutex = MUTEX_INIT;
    mutex_lock(&mutex);                                 //1. init mutex

    uart_isr_ctx[uart].tx_arg = (void*) &mutex;
    dev(uart)->tx_mask = 1;                             //2. enable tx irq
    if (xtimer_mutex_lock_timeout(&mutex, max_usec))    //3. block or timeout or noop (if isr happens during xtimer_set)
    {
        /*timeout*/
        dev(uart)->tx_mask = 0;                         //4. disable tx irq (in case of timeout)
#ifdef APS_DEBUG
        ++__aps_debug.uart_tx_timeout_cnt;
#endif
    }
    /*WAIT_UNBLOCKED:*/
    uart_isr_ctx[uart].tx_arg = 0;
}
#endif /* isr mode on */

/**
 * @brief   Wait for UART TX ready in poll and/or isr mode.
 *
 * - poll mode: tx_status is polled; the thread is not preempted.
 * - isr mode: wait for tx isr or timeout; the thread is preempted.
  *
 * @param[in] uart          UART dev id number
 * @param[in] max_loop      >0  number of poll loops
 *                          =0  no poll, no spin_blocking wait
 *                          <0  poll 'forever' (UINT32_MAX loops)
 * @param[in] max_usec      >0  the number usec before isr timeout.
 *                          =0  no isr wait
 *                          <0  isr wait forever, no timeout
 */
static int _uart_tx_wait(uart_t uart, int32_t max_poll, int32_t max_usec)
{
	(void)max_usec;
    if (!(dev(uart)->tx_status & 0x1))
    {
        while (!(dev(uart)->tx_status & 0x1) && (max_poll--!=0)) {};
#if (APS_UART_TX_WAIT_ISR != 0)         /* isr mode on */
        if (   !(dev(uart)->tx_status & 0x1) && (max_usec  !=0) )
        {
            _uart_tx_wait_isr(uart, ((max_usec>0) ? (uint32_t)max_usec : 0/*forever*/));
        }
#endif
    }
#ifdef APS_DEBUG
    __aps_debug.uart_max_loop_cnt=MAX(((int32_t)(__aps_debug.uart_max_loop_cnt)),APS_UART_TX_WAIT_POLL-max_poll);
#endif
    return (!(dev(uart)->tx_status & 0x1)) /*0:ready, 1:timeout*/;
}


void uart_write(uart_t uart, const uint8_t *data, size_t len)
{
    /* NOTE:
     * - uart_write() is a blocking primitive (own thread) but
     *   it is not recommended for a rtos to block other threads
     *   while printing/sending, hence the isr mode option.
     * - the lock is here to reduce jam in isr mode, but it
     *   can't prevent it completely if the string has been
     *   split before. (the lock could have been placed later).
     *   To prevent jam, disable isr mode, or use the logger.
     * - we do not add \r as in uart1_outch. Refer to
     *   your terminal options to toggle cr/lf or use pyterm.
     */
    assert(uart < UART_NUMOF);

    aps_uart_lock(uart);

    for (size_t i = 0; i < len; i++) {
        _uart_tx_wait(uart, tx_wait_poll(uart), tx_wait_isr(uart));
        _uart_outch_raw(uart, data[i]);
    }

    aps_uart_unlock(uart);
}

static inline void uart_tx_isr_handler(uart_t uart)
{
    if (uart_isr_ctx[uart].tx_cb)
        uart_isr_ctx[uart].tx_cb(uart_isr_ctx[uart].tx_arg, uart);
}

static inline void uart_rx_isr_handler(uart_t uart)
{
    volatile unsigned data;

    while(dev(uart)->rx_status & 0x1){
        data = dev(uart)->rx_data;
        if (uart_isr_ctx[uart].rx_cb)
            uart_isr_ctx[uart].rx_cb(uart_isr_ctx[uart].rx_arg, (uint8_t)data);
    }
}

void interrupt_handler(IRQ_UART1_RX)
{
    __enter_isr();
#ifdef APS_DEBUG
    ++(__aps_debug.irq_uart1_rx);
#endif
    uart_rx_isr_handler(0);
    __exit_isr();
}

void interrupt_handler(IRQ_UART1_TX)
{
    __enter_isr();
#ifdef APS_DEBUG
    ++(__aps_debug.irq_uart1_tx);
#endif
    uart_tx_isr_handler(0);
    __exit_isr();
}

#ifdef IRQ_UART2_RX
void interrupt_handler(IRQ_UART2_RX)
{
    __enter_isr();
#ifdef APS_DEBUG
    ++(__aps_debug.irq_uart2_rx);
#endif
    uart_rx_isr_handler(1);
    __exit_isr();
}
#endif

#ifdef IRQ_UART2_TX
void interrupt_handler(IRQ_UART2_TX)
{
    __enter_isr();
#ifdef APS_DEBUG
    ++(__aps_debug.irq_uart2_tx);
#endif
    uart_tx_isr_handler(1);
    __exit_isr();
}
#endif

#endif /*USE_PERIPH_CORTUS_UART*/
