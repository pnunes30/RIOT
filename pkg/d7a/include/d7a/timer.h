/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 * @ingroup     net
 * @file
 * @brief       Definition of D7A timer platform abstraction
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */
#ifndef D7A_TIMER_H
#define D7A_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "link_c.h"

#include "xtimer.h"
#include "msg.h"

#include "d7a.h"

/**
 * @brief   Timer time variable definition
 */
#ifndef timer_tick_t
typedef uint32_t timer_tick_t;
#endif

/*! \brief Type definition for tasks
 *
 */
#ifndef task_t
typedef void (*task_t)(void *arg);
#endif

/*! \brief Type definition for tasks
 *
 */
#ifndef error_t
typedef int error_t;
#endif


/**
 * @brief   Timer object description
 */
typedef struct
{
    //uint32_t timeout;
    timer_tick_t next_event; /**< Timer timeout in us */
    uint8_t running;         /**< Check if timer is running */
    xtimer_t dev;            /**< xtimer instance attached to this D7A timer */
    msg_t msg;               /**< message attacher to this D7A timer */
    task_t cb;               /**< callback to call when timer timeout */
    void *arg;
} timer_event;

/**
 * @brief   Initializes the timer event
 *
 * @remark  next_event must be set before starting the timer.
 *          this function initializes the callback.
 *
 * @param[in] event        Structure containing the timer event parameters
 * @param[in] callback     Function callback called at the end of the timeout
 */
error_t timer_init_event(timer_event* event, task_t callback);

/**
 * @brief   Starts and adds the timer object to the list of timer events
 *
 * @param[in] event Structure containing the timer event parameters
 * @returns error_t	SUCCESS if the event is started successfully
 */
error_t timer_add_event(timer_event* event);

/**
 * @brief   Stops and removes the timer event from the list of timer events
 *
 * @param[in] event Structure containing the timer event parameters
 */
void timer_cancel_event(timer_event* event);

/**
 * @brief  Read the current time
 *
 * @return current time
 */
__LINK_C timer_tick_t timer_get_counter_value(void);


#ifdef __cplusplus
}
#endif

#endif /* D7A_TIMER_H */
/** @} */
