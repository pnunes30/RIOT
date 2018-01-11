/*
 * Copyright (C) 2018 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @ingroup     drivers_periph_gpio
 * @{
 *
 * @file
 * @brief       Low-level GPIO driver implementation
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @}
 */

#include "cpu.h"
#include "periph/gpio.h"
#include "periph_conf.h"
#include "periph_cpu.h"

#include "machine/gpio.h"
#include "machine/ic.h"


/**
 * @brief   The STM32F0 family has 16 external interrupt lines
 */
#define EXTI_NUMOF          (16U)

/**
 * @brief   Allocate memory for one callback and argument per EXTI channel
 */
static gpio_isr_ctx_t isr_ctx[EXTI_NUMOF];

/**
 * @brief   Extract the port base address from the given pin identifier
 */
static inline GPio *_port(gpio_t pin)
{
    return (GPio *)(pin & ~(0x0f));
}

/**
 * @brief   Extract the pin number from the last 4 bit of the pin identifier
 */
static inline int _pin_num(gpio_t pin)
{
    return (pin & 0x0f);
}

int gpio_init(gpio_t pin, gpio_mode_t mode)
{
    GPio *port = _port(pin);
    int pin_num = _pin_num(pin);

    /* enable clock */
    // TODO ???

    /* set mode */
    assert(mode == GPIO_IN || mode == GPIO_OUT);
    port->dir &= ~(1 << pin_num);
    port->dir |= (mode << pin_num);    // mode => 0:input 1:output

    return 0;
}


int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank,
                  gpio_cb_t cb, void *arg)
{
    int pin_num = _pin_num(pin);
    GPio *port = _port(pin);

    /* set callback */
    isr_ctx[pin_num].cb = cb;
    isr_ctx[pin_num].arg = arg;

    /* enable clock of the IC module for external interrupt configuration */
    // TODO ???

    /* initialize pin as input */
    gpio_init(pin, mode);

    /* disable global pin interrupt */
    irq_disable();

    /* clear interrupt condition by writing the current inout register into the old _in register*/
    port->old_in = port->in;

    /* configure the active flank */
    port->edge = 0x1; // Clear all edges
    port->level_sel |= (1 << pin_num); // Select pin to interrupt

    if (flank == GPIO_RISING)
        port->rs_edge_sel = 0x1;
    else if (flank == GPIO_FALLING)
        port->fl_edge_sel = 0x1;

    /* unmask the pins interrupt channel */
    port->mask |= (1 << pin_num);

    // enable interrupt for GPIO
    irq[IRQ_GPIO1].ipl = 0;
    irq[IRQ_GPIO1].ien = 1;
    irq_enable();

    return 0;
}

void gpio_irq_enable(gpio_t pin)
{
    GPio *port = _port(pin);
    port->mask |= (1 << _pin_num(pin));
}

void gpio_irq_disable(gpio_t pin)
{
    GPio *port = _port(pin);
    port->mask &= ~(1 << _pin_num(pin));
}

int gpio_read(gpio_t pin)
{
    if (_port(pin)->dir & (1 << _pin_num(pin))) {
        return _port(pin)->out & (1 << _pin_num(pin));
    }
    else {
        return _port(pin)->in & (1 << _pin_num(pin));
    }
}

void gpio_set(gpio_t pin)
{
    _port(pin)->out = (1 << _pin_num(pin));
}

void gpio_clear(gpio_t pin)
{
    _port(pin)->out &= ~(1 << _pin_num(pin));
}

void gpio_toggle(gpio_t pin)
{
    if (gpio_read(pin)) {
        gpio_clear(pin);
    } else {
        gpio_set(pin);
    }
}

void gpio_write(gpio_t pin, int value)
{
    if (value) {
        gpio_set(pin);
    } else {
        gpio_clear(pin);
    }
}

void interrupt_handler(IRQ_GPIO)
{
    __enter_isr();
    GPio *port = (GPio*) SFRADR_GPIO1;

    /* only generate interrupts against lines which have their IMR set */
    uint32_t mask = port->mask;
    for (size_t i = 0; i < EXTI_NUMOF; i++) {
        if (isr_ctx[i].cb && (mask & (1 << i))) {

            if( (port->in & (1 << i)) != (port->old_in & (1 << i)) )
                isr_ctx[i].cb(isr_ctx[i].arg);
        }
    }
    aps_isr_end();
    __exit_isr();
}
