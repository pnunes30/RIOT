/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     pkg_d7a
 * @file
 * @brief       Implementation of D7A timer platform abstraction
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */

#include "d7a/timer.h"
#include "xtimer.h"
#include "thread.h"

#include "framework/inc/link_c.h"
#include "framework/inc/errors.h"
#include "framework/hal/inc/hwtimer.h"
#include "d7a_netdev.h"


error_t timer_init_event(timer_event* event, task_t callback)
{
    event->cb = callback;
    event->arg = NULL;
    event->running = 0;

    return 0;
}

error_t timer_add_event(timer_event* event)
{
    msg_t *msg = &(event->msg);
    msg->type = D7A_TIMEOUT_MSG_TYPE_EVENT;
    msg->content.ptr = event;

    if (event->next_event == 0)
    {
        int rtc = (msg_send_int(msg, d7a_get_pid()));
        return (rtc==1) ? SUCCESS : ERROR;
    }

    event->running = 1;
    xtimer_t *timer = &(event->dev);
    xtimer_set_msg(timer, event->next_event * HWTIMER_TICKS_1MS, msg, d7a_get_pid());

    return SUCCESS;
}

void timer_cancel_event(timer_event* event)
{
	event->running = 0;
    xtimer_remove(&(event->dev));
}

__LINK_C timer_tick_t timer_get_counter_value(void)
{
    uint32_t current_time = xtimer_now_usec();
    return (timer_tick_t)(current_time / HWTIMER_TICKS_1MS);
}

