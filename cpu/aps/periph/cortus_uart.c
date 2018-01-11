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

#include "machine/uart.h"
#include "machine/ic.h"

/**
 * @brief   Allocate memory to store the callback functions
 */
static uart_isr_ctx_t isr_ctx;

int uart_init(uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg)
{
    uint16_t integral;
    uint8_t fractional;
    uint32_t clk;

    assert(uart < UART_NUMOF);

    /* save ISR context */
    isr_ctx.rx_cb = rx_cb;
    isr_ctx.arg   = arg;

    /* No need to configure TX and RX pin */

    /* disable the clock */
    uart1->enable = 0;

    /* reset UART configuration -> defaults to 8N1 mode */
    uart1->config = 0; // 8 bits, no parity, 1 stop bit, flow control disabled

    /* calculate and apply baudrate */
    uart1->selclk = 0; // 0:50MHz, 1:25MHz, 2:12.5MHZ, 3:3.125MHz
    uart1->divider = 8*(clock_source(0)/baudrate);

    /* enable the UART module */
    uart1->enable = 1;

    /* enable RX interrupt if applicable */
    if (rx_cb) {
        irq[IRQ_UART1_RX].ien = 1;
        uart1->rx_mask = 1;  /* start Rx interrupt */
    }

    return UART_OK;
}

void uart_write(uart_t uart, const uint8_t *data, size_t len)
{
    assert(uart < UART_NUMOF);

    for (size_t i = 0; i < len; i++) {
        while (!(uart1->tx_status & UART_TX_STATUS_FIFO_NOT_EMPTY)) {}
        uart1->tx_data = data[i];
    }
}

void uart_poweron(uart_t uart)
{
    volatile int dump;

    assert(uart < UART_NUMOF);

    /* Flush the RX buffer. */
    while(uart1->rx_status)
        dump = uart1->rx_data;

    uart1->rx_mask = 1;  /* start Rx interrupt */
}

void uart_poweroff(uart_t uart)
{
    assert(uart < UART_NUMOF);
    uart1->rx_mask = 0; /* stop Rx interrupt */
    uart1->tx_mask = 0; /* stop Tx interrupt */
}

static inline void isr_handler(void)
{
    volatile unsigned data;

    while(uart1->rx_status & UART_TX_STATUS_FIFO_NOT_EMPTY){
        data = uart1->rx_data;
        if (isr_ctx.rx_cb)
            isr_ctx.rx_cb(isr_ctx.arg, (uint8_t)data);
    }

    aps_isr_end();
}

void interrupt_handler(IRQ_UART1_RX)
{
    __enter_isr();
    isr_handler();
    __exit_isr();
}
