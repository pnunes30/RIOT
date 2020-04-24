/*
 * Copyright (C) 2018 CORTUS SA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @file
 * @brief       D7A test application
 *
 * @author      Philippe Nunes <philippe.nunes@cortus.com>
 */

#include <stdio.h>

#include "shell.h"
#include "shell_commands.h"
#include "fmt.h"
#include "xtimer.h"

#if defined(MODULE_DHT)
#include "dht.h"
#include "dht_params.h"
#endif

#if defined(MODULE_GNRC)
#include "net/gnrc/pktdump.h"
#include "net/gnrc.h"
#endif

#include "d7a.h"
#include "d7ap.h"
#include "d7ap_fs.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/* set interval to 5 second */
#define INTERVAL (8 * US_PER_SEC)

#define SENSOR_FILE_ID           0x40
#define SENSOR_FILE_SIZE         4

/* Address Id is at most 8 bytes long (e.g. 16 hex chars) */
static char addr_hex_string[ID_TYPE_UID_ID_LENGTH * 2 + 1];
static char node_addr_string[ID_TYPE_UID_ID_LENGTH * 2 + 1];

uint8_t resp_len = 0;

d7ap_session_config_t session_config = {
  .qos = {
    .qos_resp_mode = SESSION_RESP_MODE_ANY,
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

static  uint32_t timeout = INTERVAL;

xtimer_t dht_timer;
bool send_dht_temp = true;
bool send_dht_hum = true;

#if defined(MODULE_DHT)
dht_t dht_dev;
#endif

#ifdef MODULE_ASYMCUTE

#include "net/asymcute.h"

#ifndef REQ_CTX_NUMOF
#define REQ_CTX_NUMOF       (8U)
#endif

#define LISTENER_PRIO       (THREAD_PRIORITY_MAIN - 1)
static char listener_stack[ASYMCUTE_LISTENER_STACKSIZE];

static asymcute_con_t d7a_connection;
static asymcute_req_t _reqs[REQ_CTX_NUMOF];
static asymcute_sub_t d7a_subscription;
static asymcute_topic_t d7a_topic_temp;
static asymcute_topic_t d7a_topic_hum;
static asymcute_topic_t d7a_topic_st;
static asymcute_topic_t d7a_sub_topic;
static asymcute_will_t d7a_will;
static uint8_t registration_status = 0;
static uint8_t init_status = 0;
static uint8_t counter_error;
static uint16_t sleep_time = 0;

/**
 * @brief   Possible states
 */
enum {
    MQTT_CONNECTING,            /**< connecting to gateway */
    MQTT_REGISTERING,           /**< registering topic */
    MQTT_DISCONNECTING,         /**< connection got disconnected */
    MQTT_PUBLISHING,            /**< data was published */
    MQTT_SUBSCRIBING,           /**< client was subscribed to topic */
    MQTT_UNSUBSCRIBING,         /**< client was unsubscribed from topic */
    MQTT_WILLTOPIC,             /**< client should provide a will topic name */
    MQTT_WILLMSG,               /**< client should provide will msg */
    MQTT_ASLEEP,                /**< client is asleep */
    MQTT_TIMEOUT,               /**< request timed out */
    MQTT_CANCELED,              /**< request was canceled */
    MQTT_REJECTED               /**< request was rejected */
};

char cli_id[MQTTSN_CLI_ID_MAXLEN];
#define topic_prefix    "d7a"
#define topic_status    "status"
#define topic_temp      "temp"
#define topic_hum       "hum"
#define topic_timeout   "timeout"

char topic_status_name[128];
char topic_temp_name[128];
char topic_hum_name[128];
char topic_timeout_name[128];
unsigned long topic_status_size = sizeof(node_addr_string) + sizeof(topic_prefix) + sizeof(topic_status);
unsigned long topic_temp_size = sizeof(node_addr_string) + sizeof(topic_prefix) + sizeof(topic_temp);
unsigned long topic_hum_size = sizeof(node_addr_string) + sizeof(topic_prefix) + sizeof(topic_hum);
unsigned long topic_timeout_size = sizeof(node_addr_string) + sizeof(topic_prefix) + sizeof(topic_timeout);

static asymcute_req_t *_get_req_ctx(void)
{
    for (unsigned i = 0; i < REQ_CTX_NUMOF; i++) {
        if (!asymcute_req_in_use(&_reqs[i])) {
            return &_reqs[i];
        }
    }
    puts("error: no request context available\n");
    return NULL;
}
#endif



static int8_t client_id;

static void _d7a_usage(void)
{
    puts("Usage: d7a <get|set|tx|dht>");
}

static void _d7a_tx_usage(void)
{
    puts("Usage: d7a tx <payload> [qos] [timeout] [access_class] [addr] [resp_len]");
}

static void _d7a_set_usage(void)
{
    puts("Usage: d7a set <devaddr|class|tx_power>");
}

static void _d7a_get_usage(void)
{
    puts("Usage: d7a get <devaddr|class|tx_power>");
}

static void _d7a_dht_usage(void)
{
    puts("Usage: d7a dht <temp|hum>");
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

    printf("\nd7a rx \"%s\" \"%s\"\n", app_data, addr_hex_string);
}

bool process_command_from_d7ap(uint8_t* payload, uint8_t len, d7ap_session_result_t result)
{
    char app_data[D7A_PAYLOAD_MAX_SIZE * 2 + 1];

    DEBUG("rssi <%idBm>, link budget <%i>\r\n", result.rx_level, result.link_budget);

    fmt_bytes_hex(app_data, payload, len);
    app_data[len * 2] = '\0';
    fmt_bytes_hex(addr_hex_string, result.addressee.id, sizeof(result.addressee.id));
    addr_hex_string[sizeof(result.addressee.id) * 2] = '\0';

    DEBUG("Unsolicited message received: %s\n", app_data);
    DEBUG("Unsolicited message received from: %s\n", addr_hex_string);

    printf("\nd7a rx \"%s\" \"%s\"\n", app_data, addr_hex_string);
    // By default, notify that a response should be sent (in practice, by the host over serial)
    return true;
}

void command_completed(uint16_t trans_id, error_t error)
{
    if (error)
        printf("The request with transId <%02x> has been transmitted with error %d\r\n", trans_id, error);
    else
        printf("The request with transId <%02x> has been transmitted successfully\r\n", trans_id);
}

static void send_packet_over_d7a(char* payload, uint8_t len)
{
    uint16_t trans_id = 0;

    if (d7ap_send(client_id, &session_config, (uint8_t *)payload, len, resp_len, &trans_id) != 0)
    {
        printf("d7ap_send failed\r\n");
        return;
    }

    printf("\nRequest accepted with transaction Id %02x:\n", trans_id);
}

void sensor_measurement(void *arg)
{
    int16_t temp, hum;
    char temp_s[10];
    char hum_s[10];

    (void)arg;
    puts("DHT temperature and humidity sensor reading");

#ifdef MODULE_DHT
    if (dht_read(&dht_dev, &temp, &hum) != DHT_OK) {
        puts("Error reading values");
        goto repeat;
    }
#else
    temp = 245;
    hum = 499;
#endif

    size_t n = fmt_s16_dfp(temp_s, temp, -1);
    temp_s[n] = '\0';
    n = fmt_s16_dfp(hum_s, hum, -1);
    hum_s[n] = '\0';

    printf("DHT values - temp: %sC - relative humidity: %s%%\n",
            temp_s, hum_s);

    uint32_t value= ((uint32_t)hum << 16) | (uint16_t)temp;
    d7ap_fs_write_file(SENSOR_FILE_ID, 0, (uint8_t*)&value, sizeof(value));

    if (send_dht_temp)
    {
#ifdef MODULE_ASYMCUTE
        /* get request context */
        asymcute_req_t *req = _get_req_ctx();
        if (req == NULL) {
            goto repeat;
        }

        /* publish data */
        size_t len = strlen(temp_s);
        if (asymcute_publish(&d7a_connection, req, &d7a_topic_temp, temp_s, len, MQTTSN_QOS_1) != ASYMCUTE_OK)
        {
            puts("error: unable to send PUBLISH message");
            goto repeat;
        }

        printf("Request %p: issued (QoS 1)\n", (void *)req);
#else
        // send measurements over D7A
        send_packet_over_d7a(temp_s, strlen(temp_s));
#endif
    }

    if (send_dht_hum)
    {
#ifdef MODULE_ASYMCUTE
        /* get request context */
        asymcute_req_t *req = _get_req_ctx();
        if (req == NULL) {
            goto repeat;
        }

        /* publish data */
        size_t len = strlen(hum_s);
        if (asymcute_publish(&d7a_connection, req, &d7a_topic_hum, hum_s, len, MQTTSN_QOS_1) != ASYMCUTE_OK)
        {
            puts("error: unable to send PUBLISH message");
            goto repeat;
        }

        printf("Request %p: issued (QoS 1)\n", (void *)req);
#else
        // send measurements over D7A
        send_packet_over_d7a(temp_s, strlen(hum_s));
#endif
    }

repeat:
    /* periodically read temp and humidity values */
    xtimer_set(&dht_timer, timeout);
    return;
}


static int _cmd_d7a(int argc, char **argv)
{
    if (argc < 2) {
        _d7a_usage();
        return 1;
    }

    if (strcmp(argv[1], "get") == 0) {
        if (argc < 3) {
            _d7a_get_usage();
            return 1;
        }

        if (strcmp("devaddr", argv[2]) == 0) {
            printf("\nDevice Address: %s\n", node_addr_string);
        }
        else if (strcmp("class", argv[2]) == 0) {
            printf("\nDevice Access class: 0x%02X\n", d7ap_get_access_class());
        }
        else if (strcmp("tx_power", argv[2]) == 0) {
            printf("\nTX power index: %d\n", d7ap_get_tx_power());
        }
        else {
            _d7a_get_usage();
            return 1;
        }
    }
    else if (strcmp(argv[1], "set") == 0) {
        if (argc < 3) {
            _d7a_set_usage();
            return 1;
        }
        if (strcmp("devaddr", argv[2]) == 0) {
            if ((argc < 4) || (strlen(argv[3]) != ID_TYPE_UID_ID_LENGTH * 2)) {
                printf("Usage: d7a set devaddr <8 hex chars>\r\n");
                return 1;
            }

            d7ap_addressee_t devaddr;
            fmt_hex_bytes(devaddr.id, argv[3]);
            //d7ap_set_dev_addr(devaddr);  //Is it expected to change the device Address from the application?
        }
        else if (strcmp("class", argv[2]) == 0) {
            if (argc < 4) {
                printf("Usage: d7a set class <access_class>\r\n");
                return 1;
            }

            d7ap_set_access_class(atoi(argv[3]));
        }
        else if (strcmp("tx_power", argv[2]) == 0) {
            if (argc < 4) {
                printf("Usage: d7a set tx_power <0..16>\r\n");
                return 1;
            }
            uint8_t power = atoi(argv[3]);
            if (power > 16) {
                printf("Usage: d7a set tx_power <0..16>\r\n");
                return 1;
            }
            d7ap_set_tx_power(power);
        }
        else {
            _d7a_set_usage();
            return 1;
        }
    }
    else if (strcmp(argv[1], "dht") == 0) {
        if (argc < 3) {
            _d7a_dht_usage();
            return 1;
        }

        if (strcmp(argv[2], "temp") == 0) {
            send_dht_temp = true;
#ifdef MODULE_ASYMCUTE  
            /* get request context */
            asymcute_req_t *req = _get_req_ctx();
            if (req == NULL) {
                return 1;
            }

            if (asymcute_connect(&d7a_connection, req, cli_id, NULL, false, NULL) != ASYMCUTE_OK) {
                puts("error: failed to issue CONNECT request");
                return 1;
            }
#endif
        }
        else if (strcmp(argv[2], "hum") == 0) {
            send_dht_hum = true;
        }
        else if (strcmp(argv[2], "off") == 0) {
            send_dht_hum = false;
            send_dht_temp = false;
            xtimer_remove(&dht_timer);
#ifdef MODULE_ASYMCUTE
            /* get request context */
            asymcute_req_t *req = _get_req_ctx();
            if (req == NULL) {
                return 1;
            }

            if (asymcute_disconnect(&d7a_connection, req, sleep_time) != ASYMCUTE_OK) {
                puts("error: failed to issue DISCONNECT request");
                return 1;
            }
#endif
            return 0;
        }
        else {
            _d7a_dht_usage();
            return 1;
        }

        sensor_measurement(NULL);
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
                    printf("Usage: d7a set devaddr <8 hex chars>\r\n");
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

static const shell_command_t shell_commands[] = {
    { "d7a", "control the d7a stack", _cmd_d7a },
    { NULL, NULL, NULL }
};

#ifdef MODULE_ASYMCUTE

static void _on_pub_evt(const asymcute_sub_t *sub, unsigned evt_type,
                        const void *data, size_t len, void *arg)
{
    (void)arg;

    if (evt_type == ASYMCUTE_PUBLISHED) {
        char *in = (char *)data;

        printf("subscription to topic #%i [%s]: NEW DATA\n",
               (int)sub->topic->id, sub->topic->name);
        printf("         data -> ");
        for (size_t i = 0; i < len; i++) {
            printf("%c", in[i]);
        }
        puts("");
        printf("              -> %u bytes\n", (unsigned)len);

        if(strcmp(sub->topic->name, topic_timeout_name) == 0)
        {
            // update timeout value
            timeout = atoi(data) * US_PER_SEC;
            printf("sensor measurement period is updated-> %lu us\n", timeout);
        }

    }
    else if (evt_type == ASYMCUTE_CANCELED) {
        printf("subscription -> topic #%i [%s]: CANCELED\n",
               (int)sub->topic->id, sub->topic->name);
    }
}


void mqtt_client_register(asymcute_topic_t *topic, const char *topic_name)
{
    /* get request context */
    asymcute_req_t *req = _get_req_ctx();
    if (req == NULL) {
        puts("error getting request context");
    }

    if (asymcute_topic_is_init(topic) == 0){
        if (asymcute_topic_init(topic, topic_name, 0) != ASYMCUTE_OK) {
            puts("error: unable to initialize topic");
            return;
        }
    }
    if (asymcute_register(&d7a_connection, req, topic) != ASYMCUTE_OK) {
        puts("error: unable to send REGISTER request\n");
        return;
    }
}


/* MQTT CLIENT INIT */
void mqtt_client_init(unsigned state)
{
    int ret = 0;

    printf("MQTT Init state : %d\r\n", state);
    switch(state) {
        case MQTT_CONNECTING:
        {
            /* get request context */
            asymcute_req_t *req = _get_req_ctx();
            if (req == NULL) {
                puts("error getting request context");
            }

            ret = asymcute_connect(&d7a_connection, req, cli_id, NULL, false, &d7a_will);
            if (ret != ASYMCUTE_OK) {
                puts("error: failed to issue CONNECT request");
                printf("error %d\n", ret);
            }
            break;
        }
        case MQTT_REGISTERING:
            mqtt_client_register(&d7a_topic_st, topic_status_name);
            mqtt_client_register(&d7a_topic_temp, topic_temp_name);
            mqtt_client_register(&d7a_topic_hum, topic_hum_name);
            registration_status = 1;
            break;
        case MQTT_SUBSCRIBING:
        {
            /* get request context */
            asymcute_req_t *req = _get_req_ctx();
            if (req == NULL) {
                puts("error getting request context");
            }

            if (asymcute_topic_is_init(&d7a_sub_topic) == 0){
                if (asymcute_topic_init(&d7a_sub_topic, topic_timeout_name, 4) != ASYMCUTE_OK) {
                    puts("error: unable to initialize topic");
                    return;
                }
            }
            if (asymcute_subscribe(&d7a_connection, req, &d7a_subscription, &d7a_sub_topic,
                            _on_pub_evt, NULL, MQTTSN_QOS_0 | MQTTSN_DUP) != ASYMCUTE_OK) {
                asymcute_topic_reset(&d7a_sub_topic);
                puts("error: unable to send SUBSCRIBE request");
                return;
            }
            break;
        }
        case MQTT_PUBLISHING:
        {
            char status[] = "online";
            size_t len = strlen(status);

            /* get request context */
            asymcute_req_t *req = _get_req_ctx();
            if (req == NULL) {
                puts("error getting request context");
            }

            if (asymcute_publish(&d7a_connection, req, &d7a_topic_st, status,
                                            len, MQTTSN_QOS_1) != ASYMCUTE_OK)
            {
                puts("error: unable to send PUBLISH message");
            }
        }
        break;
        default:
            puts("\r\nunknown event");
            break;
    }
}
static void on_err_evt(asymcute_req_t *req)
{
    printf("Message type: %d\n", req->data[1]);
    switch (req->data[1]){
        case MQTTSN_SEARCHGW:
            printf("ERROR: Can't find any gateway after %d attempts..."
                    ,ASYMCUTE_N_RETRY+1);
            break;
        case MQTTSN_CONNECT:
            printf("ERROR: Can't connect to the gateway after %d "
                    "attempts...\nRetrying...\n", ASYMCUTE_N_RETRY+1);
            counter_error++;
            if (counter_error >= 2) {
                counter_error = 0;
                printf("ERROR: CAN'T CONNECT ! \nShutting down ...\n");
                break;
            }else{
                mqtt_client_init(MQTT_CONNECTING);
            }
            break;
        case MQTTSN_DISCONNECT:
            break;
        case MQTTSN_REGISTER:
          // TODO restart the connection procedure ?
            break;
        case MQTTSN_PUBLISH:
            mqtt_client_init(MQTT_PUBLISHING);
            break;
        case MQTTSN_SUBSCRIBE:
            mqtt_client_init(MQTT_SUBSCRIBING);
            break;
        case MQTTSN_UNSUBSCRIBE:
            break;
    }
}
static void _on_con_evt(asymcute_req_t *req, unsigned evt_type)
{
    printf("MQTT Request %p: event type %d", (void *)req, evt_type);
    switch (evt_type) {
        case ASYMCUTE_TIMEOUT:
            puts("\r\nTimeout");
            break;
        case ASYMCUTE_REJECTED:
            puts("\r\nRejected by gateway");
            break;
        case ASYMCUTE_CONNECTED:
            puts("\r\nConnection to gateway established");
            if (init_status == 1) {
                mqtt_client_init(MQTT_REGISTERING);
            }
            break;
        case ASYMCUTE_DISCONNECTED:
            puts("\r\nConnection to gateway closed");
            break;
        case ASYMCUTE_ASLEEP:
            puts("\r\nDevice is asleep");
            break;
        case ASYMCUTE_REGISTERED:
            puts("\r\nTopic registered");
            if ((registration_status == 1) && (init_status == 1)){
                if (req->con->pending == NULL) // No request pending
                {
                    /* Releasing the request after using it */
                    req->con = NULL;
                    mqtt_client_init(MQTT_SUBSCRIBING);
                    registration_status = 0;
                }
            }
            req->con = NULL; // Releasing the request
            break;
        case ASYMCUTE_PUBLISHED:
            puts("\r\nData was published");
            if (init_status == 1){
                init_status = 0;
                /* Start the measurement */
                sensor_measurement(NULL);
            }
            break;
        case ASYMCUTE_SUBSCRIBED:
            puts("\r\nSubscribed topic");
            if (init_status == 1)
                mqtt_client_init(MQTT_PUBLISHING);
            break;
        case ASYMCUTE_UNSUBSCRIBED:
            puts("\r\nUnsubscribed topic");
            break;
        case ASYMCUTE_CANCELED:
            puts("\r\nCanceled");
            on_err_evt(req);
            break;
        case ASYMCUTE_WILLTOPIC:
        {
            /* get request context */
            asymcute_req_t *req = _get_req_ctx();
            if (req == NULL) {
                puts("\r\nerror getting request context");
            }
            asymcute_willtopic(&d7a_connection, req, &d7a_will,
                                              MQTTSN_QOS_0 | MQTTSN_RETAIN);
        }
        break;
        case ASYMCUTE_WILLMSG:
        {
            /* get request context */
            asymcute_req_t *req = _get_req_ctx();
            if (req == NULL) {
                puts("\r\nerror getting request context");
            }
            asymcute_willmsg(&d7a_connection, req, &d7a_will);
        }
            break;
        default:
            puts("\r\nunknown event");
            break;
    }
}
#endif

void init_node_address_string(void)
{
    d7a_address node_addr;
    d7a_get_node_address(&node_addr);

    fmt_bytes_hex(node_addr_string, node_addr.address64, sizeof(node_addr.address64));
    node_addr_string[sizeof(node_addr.address64) * 2] = '\0';
    printf("\r\nNode Address: %s\n", node_addr_string);
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
    while ((client_id = d7ap_register(&desc)) == -1);

    // dump the packet
#ifdef MODULE_GNRC
    gnrc_netreg_entry_t dump = GNRC_NETREG_ENTRY_INIT_PID(GNRC_NETREG_DEMUX_CTX_ALL,
                                                          gnrc_pktdump_pid);
    gnrc_netreg_register(GNRC_NETTYPE_UNDEF, &dump);
#endif

#if defined(MODULE_DHT)
    /* initialize DHT sensor */
    printf("Initializing DHT sensor...\t");
    if (dht_init(&dht_dev, &dht_params[0]) == DHT_OK) {
        puts("[OK]\n");
    }
    else {
        puts("[Failed]");
        return 1;
    }
#endif

    dht_timer.callback = sensor_measurement;

#ifdef MODULE_ASYMCUTE
    puts("Init Asymcute MQTT-SN library\n");

    /* setup the connection context */
    asymcute_listener_run(&d7a_connection, listener_stack, sizeof(listener_stack),
                          LISTENER_PRIO, _on_con_evt);

    /* get request context */
    asymcute_req_t *req = _get_req_ctx();
    if (req == NULL) {
        return 1;
    }

    init_node_address_string();

    registration_status = 0;
    init_status = 1;
    d7a_will.msg = "offline";
    d7a_will.msg_len = (size_t)(strlen(d7a_will.msg));
    snprintf(cli_id, sizeof(cli_id), "d7a_%s", node_addr_string);
    snprintf(topic_status_name, topic_status_size, "%s/%s/%s", topic_prefix,
             node_addr_string, topic_status);
    snprintf(topic_temp_name, topic_temp_size, "%s/%s/%s", topic_prefix,
             node_addr_string, topic_temp);
    snprintf(topic_hum_name, topic_hum_size, "%s/%s/%s", topic_prefix,
             node_addr_string, topic_hum);
    snprintf(topic_timeout_name, topic_timeout_size, "%s/%s/%s", topic_prefix,
             node_addr_string, topic_timeout);
    strncpy(d7a_will.topic, topic_status_name, topic_status_size);
    printf("cli_id: %s\n", cli_id);
    printf("Status topic: %s\n", topic_status_name);

    /* Start mqtt init */
    mqtt_client_init(MQTT_CONNECTING);

#endif

    // finally, register the sensor file, configured to use D7AActP
    d7ap_fs_file_header_t file_header = (d7ap_fs_file_header_t){
      .file_properties.action_protocol_enabled = 0,
      .file_properties.storage_class = FS_STORAGE_VOLATILE,
      .file_permissions = 0, // TODO
      .action_file_id = 0,
      .interface_file_id = 0,
      .length = SENSOR_FILE_SIZE,
      .allocated_length = SENSOR_FILE_SIZE
    };

    d7ap_fs_init_file(SENSOR_FILE_ID, &file_header, NULL);

    /* start the shell */
    printf("Starting the shell now\r\n");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}

