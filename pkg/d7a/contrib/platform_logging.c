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
 * @brief       Implementation of D7A logging platform abstraction
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 * @}
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "framework/inc/log.h"

#ifdef FRAMEWORK_LOG_ENABLED


static uint32_t counter = 0;


void log_counter_reset(void)
{
	counter = 0;
}

void log_print_string(char* format, ...)
{
    va_list args;
    va_start(args, format);
    printf("\n\r[%03ld] ", counter++);
    vprintf(format, args);
    va_end(args);
}

void log_print_stack_string(log_stack_layer_t type, char* format, ...)
{
    va_list args;
    va_start(args, format);
    printf("\n\r[%03ld] ", counter++);

    switch (type) {
        case LOG_STACK_PHY:
            printf("PHY  ");
            break;

        case LOG_STACK_DLL:
            printf("DLL  ");
            break;

        case LOG_STACK_MAC:
            printf("MAC  ");
            break;

        case LOG_STACK_NWL:
            printf("NWL  ");
            break;

        case LOG_STACK_TRANS:
            printf("TP  ");
            break;

        case LOG_STACK_SESSION:
            printf("SESSION  ");
            break;

        case LOG_STACK_D7AP:
            printf("D7A   ");
            break;

        default:
            break;
        }

    vprintf(format, args);
    va_end(args);
}

void log_print_data(uint8_t* message, uint32_t length)
{
    printf("\n\r[%03ld]", counter++);
    for( uint32_t i=0 ; i < length ; i++ )
    {
        printf(" %02X", message[i]);
    }
}

#endif //FRAMEWORK_LOG_ENABLED
