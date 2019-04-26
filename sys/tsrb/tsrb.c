/*
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup sys
 * @{
 * @file
 * @brief       thread-safe ringbuffer implementation
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 *
 * @}
 */

#include "tsrb.h"

static void _push(tsrb_t *rb, uint8_t c)
{
    rb->buf[rb->writes++ & (rb->size - 1)] = c;
}

static uint8_t _pop(tsrb_t *rb)
{
    return rb->buf[rb->reads++ & (rb->size - 1)];
}

int tsrb_get_one(tsrb_t *rb)
{
    if (!tsrb_empty(rb)) {
        return _pop(rb);
    }
    else {
        return -1;
    }
}

int tsrb_get(tsrb_t *rb, uint8_t *dst, size_t n)
{
    size_t tmp = n;
    while (tmp && !tsrb_empty(rb)) {
        *dst++ = _pop(rb);
        tmp--;
    }
    return (n - tmp);
}

int tsrb_drop(tsrb_t *rb, size_t n)
{
    size_t tmp = n;
    while (tmp && !tsrb_empty(rb)) {
        _pop(rb);
        tmp--;
    }
    return (n - tmp);
}

int tsrb_add_one(tsrb_t *rb, uint8_t c)
{
    if (!tsrb_full(rb)) {
        _push(rb, c);
        return 0;
    }
    else {
        return -1;
    }
}

int tsrb_add(tsrb_t *rb, const uint8_t *src, size_t n)
{
    size_t tmp = n;
    while (tmp && !tsrb_full(rb)) {
        _push(rb, *src++);
        tmp--;
    }
    return (n - tmp);
}

#ifdef __APS__
void tsrb_dropr(tsrb_t *rb, size_t n)
{
	rb->writes -= (rb->writes >= n) ? n : rb->writes;
	if (rb->writes < rb->reads)
		rb->writes = rb->reads;
}
void tsrb_dropl(tsrb_t *rb, size_t n)
{
	rb->reads += n;
	if (rb->writes < rb->reads)
		rb->reads = rb->writes;
}
char* tsrb_curr(const tsrb_t *rb)
{
    return &rb->buf[rb->writes & (rb->size - 1)];
}
unsigned tsrb_last(const tsrb_t *rb)
{
    return rb->writes;
}
static void _ins(tsrb_t *rb, char c, unsigned at_writes)
{
    rb->buf[at_writes & (rb->size - 1)] = c;
}
void tsrb_ins(tsrb_t *rb, const char *src, size_t n, unsigned at_writes)
{
    size_t tmp = n;
    while (tmp) {
        _ins(rb, *src++, at_writes++);
        tmp--;
    }
}
#endif
