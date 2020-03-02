/*
 * Copyright (C) 2009, Freie Universitaet Berlin (FUB).
 * Copyright (C) 2013, INRIA.
 * Copyright (C) 2015 Kaspar Schleiser <kaspar@schleiser.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     sys_shell
 * @{
 *
 * @file
 * @brief       Implementation of a very simple command interpreter.
 *              For each command (i.e. "echo"), a handler can be specified.
 *              If the first word of a user-entered command line matches the
 *              name of a handler, the handler will be called with the whole
 *              command line as parameter.
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      René Kijewski <rene.kijewski@fu-berlin.de>
 *
 * @}
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "shell.h"
#include "shell_commands.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define ETX '\x03'  /** ASCII "End-of-Text", or ctrl-C */
#if !defined(SHELL_NO_ECHO) || !defined(SHELL_NO_PROMPT)
#ifdef MODULE_NEWLIB
/* use local copy of putchar, as it seems to be inlined,
 * enlarging code by 50% */
static void _putchar(int c) {
    putchar(c);
}
#else
#define _putchar putchar
#endif
#endif

static void flush_if_needed(void)
{
#ifdef MODULE_NEWLIB
    fflush(stdout);
#endif
}

uint8_t rx_state;
typedef enum
{
    STATE_ESCAPE,
    STATE_CURSOR,
    STATE_CMD
} rx_state_t;

#define HISTORY_MAX_SIZE    5
#define MAX_COMMAND_SIZE    256
static uint8_t history_latest_cmd = 0;
static uint8_t history_top_index = 0;
static uint8_t history_index = 0;

static char history[HISTORY_MAX_SIZE][MAX_COMMAND_SIZE];
static bool history_full = false;

static shell_command_handler_t find_handler(const shell_command_t *command_list, char *command)
{
    const shell_command_t *command_lists[] = {
        command_list,
#ifdef MODULE_SHELL_COMMANDS
        _shell_command_list,
#endif
    };

    /* iterating over command_lists */
    for (unsigned int i = 0; i < ARRAY_SIZE(command_lists); i++) {

        const shell_command_t *entry;

        if ((entry = command_lists[i])) {
            /* iterating over commands in command_lists entry */
            while (entry->name != NULL) {
                if (strcmp(entry->name, command) == 0) {
                    return entry->handler;
                }
                else {
                    entry++;
                }
            }
        }
    }

    return NULL;
}

static void print_help(const shell_command_t *command_list)
{
    printf("%-20s %s\n", "Command", "Description");
    puts("---------------------------------------");

    const shell_command_t *command_lists[] = {
        command_list,
#ifdef MODULE_SHELL_COMMANDS
        _shell_command_list,
#endif
    };

    /* iterating over command_lists */
    for (unsigned int i = 0; i < ARRAY_SIZE(command_lists); i++) {

        const shell_command_t *entry;

        if ((entry = command_lists[i])) {
            /* iterating over commands in command_lists entry */
            while (entry->name != NULL) {
                printf("%-20s %s\n", entry->name, entry->desc);
                entry++;
            }
        }
    }
}

static void handle_input_line(const shell_command_t *command_list, char *line)
{
    static const char *INCORRECT_QUOTING = "shell: incorrect quoting";

    /* first we need to calculate the number of arguments */
    unsigned argc = 0;
    char *pos = line;
    int contains_esc_seq = 0;
    while (1) {
        if ((unsigned char) *pos > ' ') {
            /* found an argument */
            if (*pos == '"' || *pos == '\'') {
                /* it's a quoted argument */
                const char quote_char = *pos;
                do {
                    ++pos;
                    if (!*pos) {
                        puts(INCORRECT_QUOTING);
                        return;
                    }
                    else if (*pos == '\\') {
                        /* skip over the next character */
                        ++contains_esc_seq;
                        ++pos;
                        if (!*pos) {
                            puts(INCORRECT_QUOTING);
                            return;
                        }
                        continue;
                    }
                } while (*pos != quote_char);
                if ((unsigned char) pos[1] > ' ') {
                    puts(INCORRECT_QUOTING);
                    return;
                }
            }
            else {
                /* it's an unquoted argument */
                do {
                    if (*pos == '\\') {
                        /* skip over the next character */
                        ++contains_esc_seq;
                        ++pos;
                        if (!*pos) {
                            puts(INCORRECT_QUOTING);
                            return;
                        }
                    }
                    ++pos;
                    if (*pos == '"') {
                        puts(INCORRECT_QUOTING);
                        return;
                    }
                } while ((unsigned char) *pos > ' ');
            }

            /* count the number of arguments we got */
            ++argc;
        }

        /* zero out the current position (space or quotation mark) and advance */
        if (*pos > 0) {
            *pos = 0;
            ++pos;
        }
        else {
            break;
        }
    }
    if (!argc) {
        return;
    }

    /* then we fill the argv array */
    char *argv[argc + 1];
    argv[argc] = NULL;
    pos = line;
    for (unsigned i = 0; i < argc; ++i) {
        while (!*pos) {
            ++pos;
        }
        if (*pos == '"' || *pos == '\'') {
            ++pos;
        }
        argv[i] = pos;
        while (*pos) {
            ++pos;
        }
    }
    for (char **arg = argv; contains_esc_seq && *arg; ++arg) {
        for (char *c = *arg; *c; ++c) {
            if (*c != '\\') {
                continue;
            }
            for (char *d = c; *d; ++d) {
                *d = d[1];
            }
            if (--contains_esc_seq == 0) {
                break;
            }
        }
    }

    /* then we call the appropriate handler */
    shell_command_handler_t handler = find_handler(command_list, argv[0]);
    if (handler != NULL) {
        handler(argc, argv);
    }
    else {
        if (strcmp("help", argv[0]) == 0) {
            print_help(command_list);
        }
        else {
            printf("shell: command not found: %s\n", argv[0]);
        }
    }
}

static int readline(char *buf, size_t size)
{
    char *line_buf_ptr = buf;

    while (1) {
        if ((line_buf_ptr - buf) >= ((int) size) - 1) {
            return -1;
        }

        int c = getchar();
        if (c < 0) {
            return EOF;
        }

        /* We allow Unix linebreaks (\n), DOS linebreaks (\r\n), and Mac linebreaks (\r). */
        /* QEMU transmits only a single '\r' == 13 on hitting enter ("-serial stdio"). */
        /* DOS newlines are handled like hitting enter twice, but empty lines are ignored. */
        /* Ctrl-C cancels the current line. */
        if (c == '\r' || c == '\n' || c == ETX) {
            *line_buf_ptr = '\0';
#ifndef SHELL_NO_ECHO
            _putchar('\r');
            _putchar('\n');
#endif

            if (line_buf_ptr > buf)
            {
                // backup cmd if different from the previous one
                if (memcmp(&history[history_latest_cmd][0], buf, strlen(buf)) != 0)
                {
                    strcpy(&history[history_top_index][0], buf);
                    DEBUG("history_top_index %d  %s \r\n", history_top_index, &history[history_top_index][0]);

                    history_index = history_top_index;
                    history_latest_cmd = history_top_index;
                    if (history_top_index < (HISTORY_MAX_SIZE - 1))
                        history_top_index++;
                    else
                    {
                        history_top_index = 0;
                        history_full = true;
                    }
                }
            }

            /* return 1 if line is empty, 0 otherwise */
            return c == ETX || line_buf_ptr == buf;
        }
        /* QEMU uses 0x7f (DEL) as backspace, while 0x08 (BS) is for most terminals */
        else if (c == 0x08 || c == 0x7f) {
            if (line_buf_ptr == buf) {
                /* The line is empty. */
                continue;
            }

            *--line_buf_ptr = '\0';
            /* white-tape the character */
#ifndef SHELL_NO_ECHO
            _putchar('\b');
            _putchar(' ');
            _putchar('\b');
#endif
        }
        else if (c == 27) // escape sequence
        {
#ifndef SHELL_NO_ECHO
            //clean line
            while (line_buf_ptr > buf)
            {
                _putchar('\b');
                _putchar(' ');
                _putchar('\b');
                line_buf_ptr--;
            }
#endif
            line_buf_ptr = buf;
            rx_state = STATE_ESCAPE;
            continue;
        }
        else if ((c == 91) && (rx_state ==  STATE_ESCAPE)) // skip the [ after the escape character
        {
            rx_state = STATE_CURSOR;
            continue;
        }
        else if ((c == 'A' || c == 'B') && (rx_state ==  STATE_CURSOR))
        {
            DEBUG("history_latest %d\n", history_latest_cmd);

            if (c == 'A')
            {
                if ((history_index > history_latest_cmd) && (history_full))
                {
                     if (history_index < (HISTORY_MAX_SIZE -1))
                         history_index++;
                     else
                         history_index = 0;
                 }
                 else if (history_index < history_latest_cmd)
                     history_index++;

                 DEBUG("UP history_index %d\n", history_index);
             }
             else if (c == 'B')
             {
                 if (history_index <= history_latest_cmd)
                 {
                     if (history_index > 0)
                         history_index--;
                     else if (history_full && history_index == 0)
                         history_index =  HISTORY_MAX_SIZE - 1;
                 }
                 else if (history_index > history_latest_cmd)
                     history_index--;

                 DEBUG("DOWN history_index %d \r\n", history_index);
             }

             strcpy(buf, &history[history_index][0]);
             line_buf_ptr = buf + strlen(&history[history_index][0]);
             printf("%s", buf);

             rx_state = STATE_CMD;
             continue;
        }
        else {
            *line_buf_ptr++ = c;
            rx_state = STATE_CMD;
#ifndef SHELL_NO_ECHO
            _putchar(c);
#endif
        }
        flush_if_needed();
    }
}

static inline void print_prompt(void)
{
#ifndef SHELL_NO_PROMPT
    _putchar('>');
    _putchar(' ');
#endif

    flush_if_needed();
}

void shell_run(const shell_command_t *shell_commands, char *line_buf, int len)
{
    print_prompt();

    while (1) {
        int res = readline(line_buf, len);

        if (res == EOF) {
            break;
        }

        if (!res) {
            handle_input_line(shell_commands, line_buf);
        }

        print_prompt();
    }
}
