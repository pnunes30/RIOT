/*
 * Copyright (C) 2014 Freie Universität Berlin, Hinnerk van Bruinehsen
 *               2015 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef APS_SYS_TYPES_H
#define APS_SYS_TYPES_H

/* APS_MINICLIB is set in aps.inc.mk and generated in riotbuild.h explicitly included on the cmdline */
#ifdef MODULE_NEWLIB
# ifdef APS_MINICLIB
#  undef APS_MINICLIB
#  warning "miniclib override"
# endif
#endif

//#include <stdio.h>		/* must not be done now */
#include <stdint.h>
//#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#if defined(MODULE_NEWLIB_SYSCALLS_DEFAULT)
/* heap aliases from ldscript for newlib syscalls redefinitions (sbrk) */
#ifndef _sheap
#define _sheap __heap_bottom
#define _eheap __heap_top
#endif
#endif

#if 0
#ifndef MODULE_NEWLIB
/* newlib support for fsblkcnt_t was only recently added to the newlib git
 * repository, commit f3e587d30a9f65d0c6551ad14095300f6e81672e, 15 apr 2016.
 * Will be included in release 2.5.0, around new year 2016/2017.
 * We provide the below workaround if the used tool chain is too old. */
#ifndef _FSBLKCNT_T_DECLARED		/* for statvfs() */
#include <stdint.h>
/* Default to 32 bit file sizes on newlib platforms */
typedef uint32_t fsblkcnt_t;
typedef uint32_t fsfilcnt_t;
#define _FSBLKCNT_T_DECLARED
#endif
#endif
#endif/*0*/

//typedef      int32_t blkcnt_t;    /**< Used for file block counts */
//typedef      int32_t blksize_t;   /**< Used for block sizes */
//typedef     uint32_t clock_t;     /**< Used for system times in clock ticks */
//typedef     uint32_t clockid_t;   /**< Used for clock ID type in the clock and timer functions */
//typedef      int16_t dev_t;       /**< Used for device IDs */
typedef     uint32_t fsblkcnt_t;  /**< Used for file system block counts */
typedef     uint32_t fsfilcnt_t;  /**< Used for file system file counts */
//typedef     uint16_t gid_t;       /**< Used for group IDs */
//typedef     uint16_t id_t;        /**< Used as a general identifier */
//typedef     uint32_t ino_t;       /**< Used for file serial numbers */
//typedef     uint32_t key_t;       /**< Used for XSI interprocess communication */
//typedef     uint32_t mode_t;      /**< Used for some file attributes */
//typedef     uint16_t nlink_t;     /**< Used for link counts */
//typedef      int32_t off_t;       /**< Used for file sizes and offsets */
//typedef          int pid_t;       /**< Used for process IDs and process group IDs */
//typedef unsigned int size_t;      /**< Used for sizes of objects */
//typedef   signed int ssize_t;     /**< Used for a count of bytes or an error indication */
//typedef      int32_t suseconds_t; /**< Used for time in microseconds */
//typedef      int32_t time_t;      /**< Used for time in seconds */
//typedef     uint32_t timer_t;     /**< Used for timer ID returned by timer_create() */
//typedef     uint16_t uid_t;       /**< Used for user IDs */
//typedef     uint32_t useconds_t;  /**< Used for time in microseconds */

#ifndef snprintf
//#define snprintf(buf, sz, fmt, ...) ({ int rtc=sprintf(buf, fmt, __VA_ARGS__); assert(strlen(buf)<sz); rtc; })
#endif


#include_next <sys/types.h>

/* define unistd.h constants not present in miniclib to avoid -fuse-clib=newlib */
//#include <sys/unistd.h>
# if !defined(STDIN_FILENO) || !defined(STDOUT_FILENO) || !defined(STDOUT_FILENO)
#  define STDIN_FILENO    0       /* standard input file descriptor */
#  define STDOUT_FILENO   1       /* standard output file descriptor */
#  define STDERR_FILENO   2       /* standard error file descriptor */
# endif

#ifdef APS_MINICLIB
/* missing declarations*/
int snprintf(char * buf, size_t sz, const char* fmt, ...);
size_t strnlen(const char *s, size_t maxlen);
int close(int fd);

/* defines for vfs.h && littlefs */
/* SEEK_* are defined in stdio.h instead of sys/unistd.h */
#ifndef SEEK_SET
#define	SEEK_SET	0	/* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define	SEEK_END	2	/* set file offset to EOF plus offset */
#endif
#  include <stdlib.h>			/* strtol */
#  include <stdio.h>			/* snprintf */
#  include <string.h>			/* strlen */
#  include <stdarg.h>			/* va_list */
#  include <fcntl.h>			/* open */
#else
#  include <stdio.h>			/* implicit printf warning with newlib */
#endif /*APS_MINICLIB*/

#ifdef __cplusplus
}
#endif

#endif /* APS_SYS_TYPES_H */
