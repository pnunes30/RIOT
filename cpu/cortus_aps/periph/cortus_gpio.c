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

#define ENABLE_DEBUG    (0)
#include "debug.h"

#ifdef USE_PERIPH_CORTUS_GPIO


/**
 * @brief   Allocate memory for one callback and argument per gpio input channel
 */
static gpio_isr_ctx_t gpio_isr_ctx[GPIO_PIN_NUMOF];

void gpio_reset_level(gpio_t pin);
void gpio_reset_edge(gpio_t pin);
void gpio_irq_enable(gpio_t pin);

int gpio_init(gpio_t pin, gpio_mode_t mode)
{
	DEBUG("gpio_init(0x%x, 0x%x)\n",
			(unsigned int)pin, (unsigned int)mode);

    Gpio *port = GPIO_PORT(pin);
    int pin_num = GPIO_PIN_ID(pin);

    /* enable clock */
    // TODO ???

    /* atomic start */
    int state = irq_disable();

    /* set mode */
    assert(mode == GPIO_IN || mode == GPIO_OUT);
    port->dir &= ~(1 << pin_num);
    port->dir |= (mode << pin_num);    // mode => 0:input 1:output

    /* atomic end */
    irq_restore(state);

    return 0;
}


int gpio_init_int(gpio_t pin, gpio_mode_t mode, gpio_flank_t flank,
                  gpio_cb_t cb, void *arg)
{
	assert(mode == GPIO_IN);
	DEBUG("gpio_init_int(0x%x, 0x%x, 0x%x)\n",
			(unsigned int)pin, (unsigned int)mode, (unsigned int)flank);

    int pin_num = GPIO_PIN_ID(pin);
    Gpio *port = GPIO_PORT(pin);

    /* set callback */
    gpio_isr_ctx[pin_num].cb       = cb;
    gpio_isr_ctx[pin_num].arg      = arg;
    gpio_isr_ctx[pin_num].in_flank = flank;

    /* enable clock of the IC module for external interrupt configuration */
    // TODO ???

    /* atomic start */
    int state = irq_disable();

    /* initialize pin as input */
    gpio_init(pin, GPIO_IN);

    /* enable a new level trigger by copying the in to old_in */
    gpio_reset_level(pin);

    /* enable a new edge trigger by clearing the edge detect bit */
    gpio_reset_edge(pin);

    /* enable the level interrupt trigger type */
    port->level_sel |=  (1 << pin_num);
    //port->level_sel &=  ~(1 << pin_num);

    /* enable the edge interrupt trigger type */
    if ( (flank == GPIO_RISING) || (flank == GPIO_BOTH) )
        port->rs_edge_sel |= (1 << pin_num);
    if ( (flank == GPIO_FALLING) || (flank == GPIO_BOTH) )
        port->fl_edge_sel |= (1 << pin_num);

    /* enable global interrupt for the GPIO module */
    aps_irq[GPIO_IRQ(port)].ipl = 0;
    aps_irq[GPIO_IRQ(port)].ien = 1;

    /* unmask the pins interrupt channel */
    gpio_irq_enable(pin);

    /* atomic end */
    irq_restore(state);

    return 0;
}

int gpio_read(gpio_t pin)
{
    if (GPIO_PORT(pin)->dir & (1 << GPIO_PIN_ID(pin))) {
        return ((GPIO_PORT(pin)->out >> GPIO_PIN_ID(pin)) & 0x1);
    }
    else {
        return ((GPIO_PORT(pin)->in  >> GPIO_PIN_ID(pin)) & 0x1);
    }
}

void gpio_set(gpio_t pin)
{
	GPIO_PORT(pin)->out |=  (1 << GPIO_PIN_ID(pin));
}

void gpio_clear(gpio_t pin)
{
	GPIO_PORT(pin)->out &= ~(1 << GPIO_PIN_ID(pin));
}

void gpio_toggle(gpio_t pin)
{
	GPIO_PORT(pin)->out ^=  (1<<GPIO_PIN_ID(pin));
}

void gpio_set_old_in(gpio_t pin)
{
	GPIO_PORT(pin)->old_in |=  (1 << GPIO_PIN_ID(pin));
}

void gpio_clear_old_in(gpio_t pin)
{
	GPIO_PORT(pin)->old_in &= ~(1 << GPIO_PIN_ID(pin));
}

//#define _rs_edge_sel(pin)	((GPIO_PORT(pin)->rs_edge_sel >> GPIO_PIN_ID(pin)) & 0x1)
//#define _fl_edge_sel(pin)	((GPIO_PORT(pin)->fl_edge_sel >> GPIO_PIN_ID(pin)) & 0x1)

void gpio_reset_level(gpio_t pin)
{
	if ( !gpio_isr_ctx[GPIO_PIN_ID(pin)].cb || (GPIO_PORT(pin)->dir & (1 << GPIO_PIN_ID(pin))) )
		return; 	//nothing to do if port is out, or in but not configured.

	if (gpio_isr_ctx[GPIO_PIN_ID(pin)].in_flank == GPIO_BOTH)
	{
		/* both edges or none. trigger on any level change: old_in=in */
		if (gpio_read(pin)) {
			gpio_set_old_in(pin);
	    } else {
	    	gpio_clear_old_in(pin);
	    }
	}
	else if (gpio_isr_ctx[GPIO_PIN_ID(pin)].in_flank == GPIO_RISING)
	{
		/* rising edge only => trigger when level=1 => old_in=0 */
		gpio_clear_old_in(pin);
	}
	else if (gpio_isr_ctx[GPIO_PIN_ID(pin)].in_flank == GPIO_FALLING)
	{
		/* falling edge only => trigger when level=0 => old_in=1 */
		gpio_set_old_in(pin);
	}
	else
	{
		assert (false);
	}
}

void gpio_reset_edge(gpio_t pin)
{
	GPIO_PORT(pin)->edge &= ~(1 << GPIO_PIN_ID(pin));
}

void gpio_write(gpio_t pin, int value)
{
    if (value) {
    	gpio_set(pin);
    } else {
        gpio_clear(pin);
    }
}

void gpio_irq_enable(gpio_t pin)
{
	//DEBUG("gpio_irq_enable(0x%x)\n", pin);
	assert( !(GPIO_PORT(pin)->dir & (1 << GPIO_PIN_ID(pin))) ); //gpio must be in.
	gpio_reset_level(pin);
	gpio_reset_edge(pin);
	asm volatile ("mov r0,#0x6666\n");
	GPIO_PORT(pin)->mask |=  (1 << GPIO_PIN_ID(pin));

    uint32_t volatile _port_mask = GPIO_PORT(pin)->mask;
	uint32_t volatile _port_in   = GPIO_PORT(pin)->in;
	uint32_t volatile _port_old  = GPIO_PORT(pin)->old_in;
	uint32_t volatile _port_edge = GPIO_PORT(pin)->edge;

	DEBUG("gpio_irq_enable(pin:0x%x, mask:%02x, in:%02x, old:%02x, edge:%02x)\n",
			(unsigned int)pin, (unsigned int)_port_mask,
			(unsigned int)_port_in, (unsigned int)_port_old, (unsigned int)_port_edge);
}

void gpio_irq_disable(gpio_t pin)
{
	DEBUG("gpio_irq_disable(0x%x)\n", (unsigned int)pin);
	assert( !(GPIO_PORT(pin)->dir & (1 << GPIO_PIN_ID(pin))) ); //gpio must be in.
    GPIO_PORT(pin)->mask &= ~(1 << GPIO_PIN_ID(pin));
}

static inline void gpio_irq_handler(Gpio *port)
{
	/* freeze states which triggered us before invoking callbacks */
	uint32_t volatile _port_mask = port->mask;
	uint32_t volatile _port_in   = port->in;
	uint32_t volatile _port_old  = port->old_in;
	uint32_t volatile _port_edge = port->edge;

	DEBUG("gpio_irq_handler(port:0x%x, mask:%02x, in:%02x, old:%02x, edge:%02x)\n",
			(unsigned int)port, (unsigned int)_port_mask,
			(unsigned int)_port_in, (unsigned int)_port_old, (unsigned int)_port_edge);
	/* invoke callbacks only for configured and active pins (level or edge) */
	for (size_t i = 0; i < GPIO_PIN_NUMOF; i++) {
    	/* one shot. clear interrupt of inactive pins ? */
    	gpio_reset_edge(GPIO_PIN(port,i));
    	gpio_reset_level(GPIO_PIN(port,i));
	    if (gpio_isr_ctx[i].cb && (_port_mask & (1 << i)) ) {
	    	if( (_port_edge & (1<<i)) ||  ( (_port_in & (1<<i)) != (_port_old & (1<<i)) ) )
	    	{
	        	gpio_isr_ctx[i].cb(gpio_isr_ctx[i].arg);
	        }
	    }
	}
}

#ifdef IRQ_GPIO1
void interrupt_handler(IRQ_GPIO1)
{
    __enter_isr();
    gpio_irq_handler(gpio1);
    __exit_isr();
}
#endif /*IRQ_GPIO1*/

#ifdef IRQ_GPIO2
void interrupt_handler(IRQ_GPIO2)
{
    __enter_isr();
    gpio_irq_handler(gpio2);
    __exit_isr();
}
#endif /*IRQ_GPIO2*/

#endif /*PERIPH_CORTUS_GPIO*/
