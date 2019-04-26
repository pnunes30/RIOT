/*
 * Copyright (C) 2016 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for
 * more details.
 */

/**
 * @ingroup     cpu_native
 * @{
 *
 * @file
 * @brief       VFS wrappers for POSIX file I/O functions
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
*/

/* imported from native */

#if defined(MODULE_VFS)
#if defined(APS_MINICLIB) || !defined(MODULE_NEWLIB) || !defined(MODULE_NEWLIB_SYSCALLS_DEFAULT)
/* Note: for newlib.
 * The syscalls are reimplemented in sys/newlib_syscalls_default/syscalls.c
 * and enabled in aps.inc.mk with:
 * 		USEMODULE += newlib
 *      USEMODULE += newlib_syscalls_default
 * FIXME: multiple definition errors
 */

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vfs.h"

#ifdef MODULE_UART_STDIO
#include "uart_stdio.h"
#endif

int open(const char *name, int flags, ...)
{
    unsigned mode = 0;

    if ((flags & O_CREAT)) {
        va_list ap;
        va_start(ap, flags);
        mode = va_arg(ap, unsigned);
        va_end(ap);
    }

    int fd = vfs_open(name, flags, mode);
    if (fd < 0) {
        /* vfs returns negative error codes */
        errno = -fd;
        return -1;
    }
    return fd;
}

#ifdef APS_MINICLIB
int read(int fd, const void *dest, int count)
#else
int read(int fd, void *dest, size_t count)
#endif
{
#ifdef MODULE_UART_STDIO	//bypass uart_stdio_vfs_read for uart.!always_on case
	if (fd == STDIN_FILENO) {
		uart_start_rx_isr(STDIO_UART_DEV);
		int rtc =uart_stdio_read((void*)dest, count);
	    uart_stop_rx_isr(STDIO_UART_DEV);
	    return rtc;
	}
#endif
	int res = vfs_read(fd, (void*)dest, count);

    if (res < 0) {
        /* vfs returns negative error codes */
        errno = -res;
        return -1;
    }
    return res;
}

#ifdef APS_MINICLIB
int write(int fd, const void *src, int count)
#else
int write(int fd, const void *src, size_t count)
#endif
{
#if 0//def MODULE_UART_STDIO	//bypass uart_stdio_vfs_write for aps specific case
    if (fd == STDOUT_FILENO) {
        return uart_stdio_write(src, count);
    }
#endif

    int res = vfs_write(fd, src, count);

    if (res < 0) {
        /* vfs returns negative error codes */
        errno = -res;
        return -1;
    }
    return res;
}

int close(int fd)
{
    int res = vfs_close(fd);

    if (res < 0) {
        /* vfs returns negative error codes */
        errno = -res;
        return -1;
    }
    return res;
}

int fcntl(int fd, int cmd, ...)
{
    unsigned long arg;
    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, unsigned long);
    va_end(ap);

    int res = vfs_fcntl(fd, cmd, arg);

    if (res < 0) {
        /* vfs returns negative error codes */
        errno = -res;
        return -1;
    }
    return res;
}


off_t lseek(int fd, off_t off, int whence)
{
    int res = vfs_lseek(fd, off, whence);

    if (res < 0) {
        /* vfs returns negative error codes */
        errno = -res;
        return -1;
    }
    return res;
}

int fstat(int fd, struct stat *buf)
{
    int res = vfs_fstat(fd, buf);

    if (res < 0) {
        /* vfs returns negative error codes */
        errno = -res;
        return -1;
    }
    return 0;
}

int stat(const char *name, struct stat *st)
{
    int res = vfs_stat(name, st);

    if (res < 0) {
        /* vfs returns negative error codes */
        errno = -res;
        return -1;
    }
    return 0;
}

int unlink(const char *path)
{
    int res = vfs_unlink(path);

    if (res < 0) {
        /* vfs returns negative error codes */
        errno = -res;
        return -1;
    }
    return 0;
}

#endif /*defined(APS_MINICLIB) || !defined(MODULE_NEWLIB)*/
#endif /*MODULE_VFS*/
/** @} */
