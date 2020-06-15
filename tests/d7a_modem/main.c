/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @file
 * @brief       D7A modem application
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 */

#include <stdio.h>
#include <string.h>

#include "mutex.h"
#include "thread.h"
#include "msg.h"
#include "tsrb.h"
#include "periph/uart.h"
#include "stdio_uart.h"

#include "fmt.h"
#include "xtimer.h"

#include "net/gnrc/pktdump.h"
#include "net/gnrc.h"

#include "d7ap.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

/* UART2 Param */
#define UART_BUFSIZE        	(512U)
typedef struct {
    uint8_t rx_mem[UART_BUFSIZE];
    tsrb_t rx_buf;
} uart_ctx_t;
static uart_ctx_t ctx[UART_NUMOF];

#define MAX_COMMAND_SIZE     (256U)
char buffer[MAX_COMMAND_SIZE];

static kernel_pid_t uart_pid;
static char mod_stack[THREAD_STACKSIZE_MAIN];

/* set interval to 5 second */
#define INTERVAL (5U * US_PER_SEC)

/* Address Id is at most 8 bytes long (e.g. 16 hex chars) */
static char addr_hex_string[ID_TYPE_UID_ID_LENGTH * 2 + 1];

uint8_t resp_len = 0;

d7ap_session_config_t session_config = {
  .qos = {
    .qos_resp_mode = SESSION_RESP_MODE_NO,
    .qos_retry_mode = SESSION_RETRY_MODE_NO
  },
  .dormant_timeout = 0,
  .addressee = {
    .ctrl = {
      .nls_method = AES_NONE,
      .id_type = ID_TYPE_NOID,
    },
    .access_class = 0x01,
    .id = 0
  }
};

static int8_t client_id;

static void _d7a_usage(void)
{
    //puts("Usage: d7a <get|tx>");
}

static void _d7a_tx_usage(void)
{
    //puts("Usage: d7a tx <payload> [qos] [timeout] [access_class] [addr] [resp_len]");
}

static void _d7a_get_usage(void)
{
    //puts("Usage: d7a get <devaddr|class|tx_power>");
}

void process_response_from_d7ap(uint16_t trans_id, uint8_t* payload, uint8_t len, d7ap_session_result_t result)
{
    (void)trans_id;
    char app_data[D7A_PAYLOAD_MAX_SIZE * 2 + 1];

    DEBUG("rssi <%idBm>, link budget <%i>\r\n", result.rx_level, result.link_budget);

    fmt_bytes_hex(app_data, payload, len);
    app_data[len * 2] = '\0';
    fmt_bytes_hex(addr_hex_string, result.addressee.id, sizeof(result.addressee.id));
    addr_hex_string[sizeof(result.addressee.id) * 2] = '\0';

    DEBUG("Response received: %s\n", app_data);
    DEBUG("Response received from: %s\n", addr_hex_string);

    //printf("d7a rx \"%s\" \"%s\"\n", app_data, addr_hex_string);
}

bool process_command_from_d7ap(uint8_t* payload, uint8_t len, d7ap_session_result_t result)
{
    char app_data[D7A_PAYLOAD_MAX_SIZE * 2 + 1];
    int length = 0;

    DEBUG("rssi <%idBm>, link budget <%i>\r\n", result.rx_level, result.link_budget);

    fmt_bytes_hex(app_data, payload, len);
    app_data[len * 2] = '\0';
    fmt_bytes_hex(addr_hex_string, result.addressee.id, sizeof(result.addressee.id));
    addr_hex_string[sizeof(result.addressee.id) * 2] = '\0';

    DEBUG("Unsolicited message received: %s\n", app_data);
    DEBUG("Unsolicited message received from: %s\n", addr_hex_string);

    length = sprintf(buffer, "d7a rx %s %s\r\n\0", app_data, addr_hex_string);
    uart_write(UART_DEV(1), buffer, length);
    // By default, notify that a response should be sent (in practice, by the host over serial)

    return true;
}

void command_completed(uint16_t trans_id, error_t error)
{
    if (error) {
        //printf("The request with transId <%d> has been transmitted with error %d\r\n", trans_id, error);
    }
    else {
        //printf("The request with transId <%d> has been transmitted successfully\r\n", trans_id);
    }
}

static void send_packet_over_d7a(char* payload, uint8_t len)
{
    uint16_t trans_id = 0;

    if (d7ap_send(client_id, &session_config, (uint8_t *)payload, len, resp_len, &trans_id) != 0)
    {
        //printf("d7ap_send failed\r\n");
        return;
    }

    //printf("Request accepted with transaction Id %d:\n", trans_id);
}

static int _cmd_d7a(int argc, char **argv)
{
    if (argc < 2) {
        _d7a_usage();
        return 1;
    }

    for(int i=0; i < argc; i++)
    {
    	//printf("argc %d arg %s \n", i, argv[i]);
    }

    if (strcmp(argv[1], "get") == 0) {
        if (argc < 3) {
            _d7a_get_usage();
            return 1;
        }

        if (strcmp("devaddr", argv[2]) == 0) {
            d7ap_addressee_t devaddr;
            d7ap_get_dev_addr(devaddr.id);

            uint8_t len = d7ap_addressee_id_length(devaddr.ctrl.id_type);
            fmt_bytes_hex(addr_hex_string, devaddr.id, len);
            addr_hex_string[len * 2] = '\0';
            //printf("Device Address: %s\n", addr_hex_string);
        }
        else if (strcmp("class", argv[2]) == 0) {
        	//printf("Device Access class: 0x%02X\n", d7ap_get_access_class());
        }
        else if (strcmp("tx_power", argv[2]) == 0) {
        	//printf("TX power index: %d\n", d7ap_get_tx_power());
        }
        else {
            _d7a_get_usage();
            return 1;
        }
    }
    else if (strcmp(argv[1], "tx") == 0) {
        if (argc < 3) {
            _d7a_tx_usage();
            return 1;
        }

        /* handle overriding parameters */
        if ((argc > 3) && ((argc - 3)%2))
        {
            _d7a_tx_usage();
            return 1;
        }

        session_config.addressee.ctrl.id_type = ID_TYPE_NOID;


        uint8_t index = 3;
        while (index < argc) {
            if (strcmp(argv[index], "qos") == 0) {
                session_config.qos.raw = atoi(argv[index+1]);
                index += 2;
            }
            else if (strcmp(argv[index], "timeout") == 0) {
                session_config.dormant_timeout = atoi(argv[index+1]);
                index += 2;
            }
            else if (strcmp(argv[index], "access_class") == 0) {
                session_config.addressee.access_class = atoi(argv[index+1]);
                index += 2;
            }
            else if (strcmp(argv[index], "addr") == 0) {
                if (strlen(argv[index+1]) != ID_TYPE_UID_ID_LENGTH * 2) {
                    //printf("Usage: d7a set devaddr <8 hex chars>\r\n");
                    return 1;
                }
                fmt_hex_bytes(session_config.addressee.id, argv[index+1]);
                session_config.addressee.ctrl.id_type = ID_TYPE_UID;
                index += 2;
            }
            else if (strcmp(argv[index], "resp_len") == 0) {
                resp_len = atoi(argv[index+1]);
                index += 2;
            }
            else {
                _d7a_tx_usage();
                return 1;
            }
        }

        char payload[D7A_PAYLOAD_MAX_SIZE];
        size_t len = fmt_hex_bytes((uint8_t*)payload, argv[2]);
        send_packet_over_d7a(payload, len);

        return 0;
    }
    else {
        _d7a_usage();
        return 1;
    }

    return 0;
}

static void handle_input_line(char *line)
{
	static const char *INCORRECT_QUOTING = "shell: incorrect quoting";

	//printf("line = %s\n", line);

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
    _cmd_d7a(argc, argv);
}

static void rx_cb(void *arg, uint8_t data)
{
    uart_t dev = (uart_t)arg;

    tsrb_add_one(&(ctx[dev].rx_buf), data);

    if ((data == '\n') || (data == '\0')) {
    	msg_t msg;
        msg.content.value = (uint32_t)dev;
        msg_send(&msg, uart_pid);
    }
}

static void *uart_recv(void *arg)
{
    (void)arg;
    msg_t msg;
    msg_t msg_queue[8];
    msg_init_queue(msg_queue, 8);
    char received_cmd[MAX_COMMAND_SIZE];

    while (1) {
        msg_receive(&msg);
        uart_t dev = (uart_t)msg.content.value;
        uint16_t i = 0;
        char c;
        do {
            c = (int)tsrb_get_one(&(ctx[dev].rx_buf));
            if (c == '\n')
                break;

            received_cmd[i] = c;
        i++;
        } while (i <= MAX_COMMAND_SIZE);
        received_cmd[i] = '\0';
        //printf("\n%s\n", received_cmd);
        handle_input_line(received_cmd);
        i = 0;
    }
    /* this should never be reached */
    return NULL;
}

int main(void)
{
    d7ap_resource_desc_t desc = {
        .receive_cb = process_response_from_d7ap,
        .transmitted_cb = command_completed,
        .unsolicited_cb = process_command_from_d7ap
    };

    printf("D7A test application PID[%d] \r\n", sched_active_pid);

    // Register this application as a D7A client, this is a blocking call
    while ((client_id = d7ap_register(&desc)) < 0);
    printf("D7A Client ID : %d \r\n", client_id);

    /* initialize ringbuffers */
    for (unsigned i = 0; i < UART_NUMOF; i++) {
        tsrb_init(&(ctx[i].rx_buf), ctx[i].rx_mem, UART_BUFSIZE);
    }

    /* initialize UART 2*/
    int dev = 1;    /* Can't use UART1 on ciot25 demo board */
	int res = 0;
	uint32_t baud = 115200;
    res = uart_init(UART_DEV(dev), baud, rx_cb, (void *)dev);
    if (res != UART_OK) {
        return 1;
    }

    /* start the uart 2 thread */
    uart_pid = thread_create(mod_stack, sizeof(mod_stack),
        (THREAD_PRIORITY_MAIN - 1), 0, uart_recv, NULL, "uart_resp");

    return 0;
}
