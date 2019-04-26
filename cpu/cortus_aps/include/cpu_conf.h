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

/* CORTUS EXTRA DEBUG */
#define APS_DEBUG

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
# if defined(APS_DEBUG) || defined(DEVELHELP)
#   define THREAD_STACKSIZE_IDLE      ((512)+THREAD_EXTRA_STACKSIZE_PRINTF)
# else
#   define THREAD_STACKSIZE_IDLE      (256)
#endif
#endif

/**
 * @brief   Stack size used for the exception (ISR) stack
 * @{
 */
#ifndef ISR_STACKSIZE
#define ISR_STACKSIZE                   (0) /*(512U) unused */
#endif
/** @} */

#ifndef GNRC_PKTBUF_SIZE
/* FIXME: default for ipv6, tune this to reduce net stack size - make it MTU dependent */
#define GNRC_PKTBUF_SIZE                 (6144)  //(2560)
#endif

/**
 * @brief    vfs_mounttab hook implementation (see vfs.h)
 */
#if defined(MODULE_VFS) && defined(MODULE_SHELL)
extern unsigned _vfs_mounttab_start[], _vfs_mounttab_end[];
#define __VFS_MOUNTTAB_SECTION__	__attribute__((section(".data.vfs_mounttab"), used))
#define VFS_MOUNTTAB_SZ	(unsigned int)(((unsigned)_vfs_mounttab_end-(unsigned)_vfs_mounttab_start) / sizeof(vfs_mount_t))
#define VFS_MOUNTTAB	((vfs_mount_t*)_vfs_mounttab_start)
#endif
/** @} */

#if defined(MODULE_VFS) && defined(MODULE_TMPFS)
/**
 * @name    tmpfs configuration
 *          https://github.com/RIOT-OS/RIOT/pull/8553(.patch)
 *          https://github.com/OTAkeys/RIOT/tree/pr/tempfs
 *
 *          max memory used will be BUF_SIZE*MAX_BUF
 * @{
 */
#define TMPFS_MAX_FILES     (40)
#define TMPFS_BUF_SIZE      (8)
#define TMPFS_MAX_BUF       (64)
#endif




#ifdef __cplusplus
}
#endif

#endif /* CPU_CONF_H */
/** @} */
