/*
 * Copyright (C) 2017 CORTUS
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_aps
 * @{
 */

/**
 * @file
 * @brief       Implementation specific CPU configuration options
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 */

#ifndef CPU_CONF_H
#define CPU_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Kernel configuration
 *
 * Since printf seems to get memory allocated by the linker/avr-libc the stack
 * size tested successfully even with pretty small stacks.
 * @{
 */
#ifndef THREAD_EXTRA_STACKSIZE_PRINTF
#define THREAD_EXTRA_STACKSIZE_PRINTF   (512)
#endif

#ifndef THREAD_STACKSIZE_DEFAULT
#   define THREAD_STACKSIZE_DEFAULT   (1024)
#endif

#ifndef THREAD_STACKSIZE_IDLE
#   define THREAD_STACKSIZE_IDLE      (256)
#endif

/**
 * @brief   Stack size used for the exception (ISR) stack
 * @{
 */
#ifndef ISR_STACKSIZE
#define ISR_STACKSIZE                   (512U)
#endif
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* CPU_CONF_H */
/** @} */
