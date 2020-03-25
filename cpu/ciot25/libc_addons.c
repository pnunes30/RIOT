/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */
/**
 * @ingroup     cpu_aps
 * @{
 *
 * @file
 * @brief       Implementation of defined function but not implemented in miniclib
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 *
 * @}
 */

#include <sys/types.h>
#include <stdbool.h>
#include "assert.h"

#if defined(APS_MINICLIB)

#ifndef atoi
/* atoi is defined but not implemented in miniclib */
#if !defined(strtol)
#define _isdigit(c)		(c >= '0' && c <= '9')
#define _isspace(c)		(c == ' ' || c <= '\t' || c <= '\n' || c <= '\r' || c <= '\f' || c <= '\v')
int atoi(const char *c)
{
    int value = 0;
    int sign = 1;
    while (_isspace(*c))
    {
    	c++;
    }
    if( *c == '+' || *c == '-' )
    {
        sign = (*c == '-') ? -1 : 1;
        c++;
    }
    while (_isdigit(*c))
    {
        value = value*10 + (int)(*c-'0');
        c++;
    }
    return (value * sign);
}
#else
/* but strtol yes!... */
int atoi(const char *c)
{
	return (int)strtol(c, NULL, 10);
}
#endif
#endif /*atoi*/

#ifndef snprintf
/* snprintf is not implemented in miniclib, but sprintf yes... why the hell? */
int snprintf(char * buf, size_t sz, const char* fmt, ...)
{
//#warning "FIXME: unsafe snprintf workaround and unreliable sprintf(va_list)..."
	va_list argptr;
	va_start(argptr, fmt);
	int rtc = vsprintf(buf, fmt, argptr);	 //unsafe //fmt not a string literal warning
	va_end(argptr);
	assert( strlen(buf) < sz);   //crash afterwards if buffer overflow...
	return rtc;
}
#endif

#if !defined(strspn) || !defined(strcspn)		/* for littlefs */
size_t my_strxspn(const char* cs, const char* ct, bool stop_cond) {
  size_t n;
  const char* p;
  for(n=0; *cs; cs++, n++) {
    for(p=ct; *p && *p != *cs; p++)
      ;
    if ( (!*p) == stop_cond)
      break;
  }
  return n;
}
#endif

#ifndef strspn		/* for littlefs */
size_t strspn(const char* cs, const char* ct) {
	return my_strxspn(cs, ct, true);
}
#endif

#ifndef strcspn		/* for littlefs */
size_t strcspn(const char* cs, const char* ct) {
	return my_strxspn(cs, ct, false);
}
#endif

#ifndef strnlen		/* for devfs & constfs. not implemented in miniclib */
size_t strnlen(const char *str, size_t maxlen) {
    const char *start = str;
    while (maxlen-- > 0 && *str)
        str++;
    return str - start;
}
#endif

#endif /*(APS_MINICLIB)*/

