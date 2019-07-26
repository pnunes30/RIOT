/*
 * Copyright (C) 2018 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_asymcute
 * @{
 *
 * @file
 * @brief       Asynchronous MQTT-SN implementation
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <limits.h>
#include <string.h>

#include "log.h"
#include "random.h"
#include "byteorder.h"

#include "net/asymcute.h"

#ifdef MODULE_D7A
#include "d7a.h"
#endif

#define ENABLE_DEBUG            (0)
#include "debug.h"

#define PROTOCOL_VERSION        (0x01)

#define RETRY_TO                (ASYMCUTE_T_RETRY * US_PER_SEC)
#define KEEPALIVE_TO            (ASYMCUTE_KEEPALIVE_PING * US_PER_SEC)

#define VALID_PUBLISH_FLAGS     (MQTTSN_QOS_1 | MQTTSN_DUP | MQTTSN_RETAIN)
#define VALID_SUBSCRIBE_FLAGS   (MQTTSN_QOS_1 | MQTTSN_DUP)

#define MINLEN_CONNACK          (3U)
#define MINLEN_DISCONNECT       (2U)
#define MINLEN_REGACK           (7U)
#define MINLEN_PUBACK           (7U)
#define MINLEN_SUBACK           (8U)
#define MINLEN_UNSUBACK         (4U)
#define MINLEN_GWINFO           (3U)
#define MINLEN_WILLTOPICREQ     (2U)
#define MINLEN_WILLMSGREQ       (2U)

#define IDPOS_REGACK            (4U)
#define IDPOS_PUBACK            (4U)
#define IDPOS_SUBACK            (5U)
#define IDPOS_UNSUBACK          (2U)

#define LEN_PINGRESP            (2U)

#define BROADCAST_RADIUS        (1U)

/* Internally used connection states */
enum {
    UNINITIALIZED = 0,      /**< connection context is not initialized */
    NOTCON,                 /**< not connected to any gateway */
    SEARCHING_GW,           /**< searching for an active gateway */
    CONNECTING,             /**< connection is being setup */
    CONNECTED,              /**< connection is established */
    TEARDOWN,               /**< connection is being torn down */
    SLEEPING,               /**< connection is up, device wants to be asleep */
    SLEEP,                  /**< connection is up, device is in asleep state */
};

/* the main handler thread needs a stack and a message queue */
static event_queue_t _queue;
static char _stack[ASYMCUTE_HANDLER_STACKSIZE];

static asymcute_req_t search_gw_req = {
    .data = { 3, MQTTSN_SEARCHGW, BROADCAST_RADIUS },
    .data_len = 3,
    .broadcast = true
};

/* necessary forward function declarations */
static void _on_req_timeout(void *arg);

static size_t _len_set(uint8_t *buf, size_t len)
{
    if (len < (0xff - 7)) {
        buf[0] = len + 1;
        return 1;
    }
    else {
        buf[0] = 0x01;
        byteorder_htobebufs(&buf[1], (uint16_t)(len + 3));
        return 3;
    }
}

static size_t _len_get(uint8_t *buf, size_t *len)
{
    if (buf[0] != 0x01) {
        *len = (uint16_t)buf[0];
        return 1;
    }
    else {
        *len = byteorder_bebuftohs(&buf[1]);
        return 3;
    }
}

/* @pre con is locked */
static uint16_t _msg_id_next(asymcute_con_t *con)
{
    if (++con->last_id == 0) {
        return ++con->last_id;
    }
    return con->last_id;
}

static void send_unicast(asymcute_con_t *con, const void *data, size_t len)
{
#if defined(MODULE_D7A) && !defined(MODULE_GNRC_SOCK_UDP)
    if (con->gw_count == 0)
    {
        DEBUG("ERROR, unicast not possible when gateway is not connected");
        return;
    }

    d7a_unicast(data, len, &con->gateway[con->gw_connected].addr);
#else
    sock_udp_send(&con->sock, data, len, &con->server_ep);
#endif
}

static void send_broadcast(asymcute_con_t *con, const void *data, size_t len)
{
#if defined(MODULE_D7A) && !defined(MODULE_GNRC_SOCK_UDP)
    (void)con;
    d7a_broadcast(data, len);
#else
    sock_udp_send(&con->sock, data, len, NULL); // NULL remote not yet supported
#endif
}

/* @pre con is locked */
static asymcute_req_t *_req_preprocess(asymcute_con_t *con,
                                       size_t msg_len, size_t min_len,
                                       const uint8_t *buf, unsigned id_pos)
{
    /* verify message length */
    if (msg_len < min_len) {
        return NULL;
    }

     uint16_t msg_id = (buf == NULL) ? 0 : byteorder_bebuftohs(&buf[id_pos]);

    asymcute_req_t *res = NULL;
    asymcute_req_t *iter = con->pending;
    if (iter == NULL) {
        return NULL;
    }
    if (iter->msg_id == msg_id) {
        res = iter;
        con->pending = iter->next;
    }
    while (iter && !res) {
        if (iter->next && (iter->next->msg_id == msg_id)) {
            res = iter->next;
            iter->next = iter->next->next;
        }
        iter = iter->next;
    }

    if (res) {
        //res->con = NULL;
        event_timeout_clear(&res->to_timer);
    }
    return res;
}

/* @pre con is locked */
static void _req_remove(asymcute_con_t *con, asymcute_req_t *req)
{
    if (con->pending == req) {
        con->pending = con->pending->next;
    }
    for (asymcute_req_t *cur = con->pending; cur; cur = cur->next) {
        if (cur->next == req) {
            cur->next = cur->next->next;
        }
    }
    req->con = NULL;
}

/* @pre con is locked */
static void _compile_sub_unsub(asymcute_req_t *req, asymcute_con_t *con,
                               asymcute_sub_t *sub, uint8_t type)
{
    size_t topic_len = strlen(sub->topic->name);
    size_t pos = _len_set(req->data, (topic_len + 4));


    req->msg_id = _msg_id_next(con);
    req->data[pos] = type;
    req->data[pos + 1] = sub->topic->flags;
    byteorder_htobebufs(&req->data[pos + 2], req->msg_id);
    memcpy(&req->data[pos + 4], sub->topic->name, topic_len);
    req->data_len = (pos + 4 + topic_len);
    req->arg = (void *)sub;
}

static void _req_resend(asymcute_req_t *req, asymcute_con_t *con)
{
    event_timeout_set(&req->to_timer, RETRY_TO);
    if (req->broadcast)
        send_broadcast(con, req->data, req->data_len);
    else
        send_unicast(con, req->data, req->data_len);
}

/* @pre con is locked */
static void _req_send(asymcute_req_t *req, asymcute_con_t *con,
                      asymcute_to_cb_t cb)
{
    /* initialize request */
    req->con = con;
    req->cb = cb;
    req->retry_cnt = ASYMCUTE_N_RETRY;
    event_callback_init(&req->to_evt, _on_req_timeout, (void *)req);
    event_timeout_init(&req->to_timer, &_queue, &req->to_evt.super);
    /* add request to the pending queue (if non-con request) */
    req->next = con->pending;
    con->pending = req;

    /* send request */
    _req_resend(req, con);
}

static void _req_send_once(asymcute_req_t *req, asymcute_con_t *con)
{
    send_unicast(con, req->data, req->data_len);
    mutex_unlock(&req->lock);
}

static void _req_cancel(asymcute_req_t *req)
{
    asymcute_con_t *con = req->con;
    event_timeout_clear(&req->to_timer);
    req->con = NULL;
    mutex_unlock(&req->lock);
    con->user_cb(req, ASYMCUTE_CANCELED);
}

static void _sub_cancel(asymcute_sub_t *sub)
{
    sub->cb(sub, ASYMCUTE_CANCELED, NULL, 0, sub->arg);
    sub->topic = NULL;
}

/* @pre con is locked */
static void _disconnect(asymcute_con_t *con, uint8_t state)
{
    if (con->state == CONNECTED) {
        /* cancel all pending requests */
        event_timeout_clear(&con->keepalive_timer);
        for (asymcute_req_t *req = con->pending; req; req = req->next) {
            _req_cancel(req);
        }
        con->pending = NULL;
        for (asymcute_sub_t *sub = con->subscriptions; sub; sub = sub->next) {
            _sub_cancel(sub);
        }
        con->subscriptions = NULL;
    }
    con->state = state;
}

static void _on_req_timeout(void *arg)
{
    asymcute_req_t *req = (asymcute_req_t *)arg;

    DEBUG("_on_req_timeout resend the request \n");

    /* only process the timeout, if the request is still active */
    if (req->con == NULL) {
        return;
    }

    if (req->retry_cnt--) {
        /* resend the packet */
        _req_resend(req, req->con);
        return;
    }
    else {
        asymcute_con_t *con = req->con;
        mutex_lock(&con->lock);
        _req_remove(con, req);
        /* communicate timeout to outer world */
        unsigned ret = ASYMCUTE_TIMEOUT;
        if (req->cb) {
            ret = req->cb(con, req);
        }
        mutex_unlock(&req->lock);
        mutex_unlock(&con->lock);
        con->user_cb(req, ret);
    }
}

static unsigned _on_con_timeout(asymcute_con_t *con, asymcute_req_t *req)
{
    (void)req;

    if(con->state == SEARCHING_GW)
    {
        // unlock the next request containing the connect message
        mutex_unlock(&req->next->lock);
    }

    con->state = NOTCON;
    return ASYMCUTE_TIMEOUT;
}

static unsigned _on_discon_timeout(asymcute_con_t *con, asymcute_req_t *req)
{
    (void)req;

    con->state = NOTCON;
    return ASYMCUTE_DISCONNECTED;
}

static unsigned _on_suback_timeout(asymcute_con_t *con, asymcute_req_t *req)
{
    (void)con;

    /* reset the subscription context */
    asymcute_sub_t *sub = (asymcute_sub_t *)req->arg;
    sub->topic = NULL;
    return ASYMCUTE_TIMEOUT;
}

static void _on_keepalive_evt(void *arg)
{
    asymcute_con_t *con = (asymcute_con_t *)arg;

    mutex_lock(&con->lock);

    if (con->state != CONNECTED)
    {
        if(con->state != SLEEP)
        {
            mutex_unlock(&con->lock);
            return;
        }
    }
    if (con->keepalive_retry_cnt) {
        /* (re)send keep alive ping and set dedicated retransmit timer */
        if (con->state == SLEEP)
        {
            size_t id_len = strlen(con->cli_id);
            uint8_t ping[ASYMCUTE_ID_MAXLEN + 3];
            ping[0] = (uint8_t)(id_len + 2);
            ping[1] = MQTTSN_PINGREQ;
            memcpy(&ping[2], con->cli_id, id_len);
            send_unicast(con, ping, (size_t)(ping[0]));
        }
        else
        {
            uint8_t ping[2] = { 2, MQTTSN_PINGREQ };
            send_unicast(con, ping, sizeof(ping));
        }
        con->keepalive_retry_cnt--;
        event_timeout_set(&con->keepalive_timer, RETRY_TO);
        mutex_unlock(&con->lock);
    }
    else {
        _disconnect(con, NOTCON);
        mutex_unlock(&con->lock);
        con->user_cb(NULL, ASYMCUTE_DISCONNECTED);
    }
}

static void _on_gw_info(asymcute_con_t *con, const uint8_t *data, size_t len)
{
    mutex_lock(&con->lock);
    asymcute_req_t *req = _req_preprocess(con, len, MINLEN_GWINFO, NULL, 0);
    /* if not use later by the application, releasing the request
       as it should be done in the _req_preprocess function */
    req->con = NULL;

    //asymcute_req_t *req = con->pending;
    if ((req == NULL) || (con->state != SEARCHING_GW)) {
        DEBUG("_on_gw_info ERROR req = NULL\n");
        mutex_unlock(&con->lock);
        return;
    }

    // store the gw info in the list
    con->gateway[con->gw_count].gw_id = data[2];
    if (len == 11)
    {
        memcpy(&con->gateway[con->gw_count].addr, &data[3], sizeof(d7a_address));
    }
    else
    {
        memcpy(&con->gateway[con->gw_count].addr, &con->remote_addr, sizeof(d7a_address));
    }

    DEBUG("\nGW info store new GW id %d \n", con->gateway[con->gw_count].gw_id);
    con->gw_count++;

    /* Send the CONNECT message */
    req = req->next;
    con->state = CONNECTING;
    con->pending = NULL;

    _req_send(req, con, _on_con_timeout);

    mutex_unlock(&con->lock);
}


static void _on_connack(asymcute_con_t *con, const uint8_t *data, size_t len)
{
    mutex_lock(&con->lock);
    asymcute_req_t *req = _req_preprocess(con, len, MINLEN_CONNACK, NULL, 0);
    /* if not use later by the application, releasing the request
       as it should be done in the _req_preprocess function */
    req->con = NULL;

    if (req == NULL) {
        DEBUG("_on_connack ERROR req = NULL\n");
        mutex_unlock(&con->lock);
        return;
    }

    /* check return code and mark connection as established */
    unsigned ret = ASYMCUTE_REJECTED;
    if (data[2] == MQTTSN_ACCEPTED) {
        con->state = CONNECTED;
        /* start keep alive timer */
        event_timeout_set(&con->keepalive_timer, KEEPALIVE_TO);
        ret = ASYMCUTE_CONNECTED;
    }

    mutex_unlock(&req->lock);
    mutex_unlock(&con->lock);
    con->user_cb(req, ret);
}

static void _on_disconnect(asymcute_con_t *con, size_t len)
{
    mutex_lock(&con->lock);
    /* we might have triggered the DISCONNECT process ourselves, so make sure
     * the pending request is being handled */
    asymcute_req_t *req = _req_preprocess(con, len, MINLEN_DISCONNECT, NULL, 0);
    /* if not use later by the application, releasing the request
       as it should be done in the _req_preprocess function */
    req->con = NULL;

    if (con->state != SLEEPING) {
        /* put the connection back to NOTCON in any case and let the user know */
        _disconnect(con, NOTCON);
        if (req) {
            mutex_unlock(&req->lock);
        }
        mutex_unlock(&con->lock);
        con->user_cb(req, ASYMCUTE_DISCONNECTED);
    }
    else {
        con->state = SLEEP;
        mutex_unlock(&req->lock);
        mutex_unlock(&con->lock);
        con->user_cb(req, ASYMCUTE_ASLEEP);
    }

}

static void _on_pingreq(asymcute_con_t *con)
{
    /* simply reply with a PINGRESP message */
    mutex_lock(&con->lock);
    uint8_t resp[2] = { LEN_PINGRESP, MQTTSN_PINGRESP };
    send_unicast(con, resp, sizeof(resp));
    mutex_unlock(&con->lock);
}

static void _on_pingresp(asymcute_con_t *con)
{
    mutex_lock(&con->lock);
    /* only handle ping resp message if we are actually waiting for a reply */
    if (con->keepalive_retry_cnt < ASYMCUTE_N_RETRY) {
        event_timeout_clear(&con->keepalive_timer);
        con->keepalive_retry_cnt = ASYMCUTE_N_RETRY;
        event_timeout_set(&con->keepalive_timer, KEEPALIVE_TO);
    }
    mutex_unlock(&con->lock);
}

static void _on_regack(asymcute_con_t *con, const uint8_t *data, size_t len)
{
    mutex_lock(&con->lock);
    asymcute_req_t *req = _req_preprocess(con, len, MINLEN_REGACK,
                                          data, IDPOS_REGACK);
    if (req == NULL) {
        mutex_unlock(&con->lock);
        return;
    }

    /* check return code */
    unsigned ret = ASYMCUTE_REJECTED;
    if (data[6] == MQTTSN_ACCEPTED) {
        /* finish the registration by applying the topic id */
        asymcute_topic_t *topic = (asymcute_topic_t *)req->arg;
        topic->id = byteorder_bebuftohs(&data[2]);
        topic->con = con;
        ret = ASYMCUTE_REGISTERED;
    }

    /* finally notify the user and free the request */
    mutex_unlock(&req->lock);
    mutex_unlock(&con->lock);
    con->user_cb(req, ret);
}

static void _on_publish(asymcute_con_t *con, uint8_t *data,
                        size_t pos, size_t len)
{
    /* verify message length */
    if (len < (pos + 6)) {
        return;
    }

    uint16_t topic_id = byteorder_bebuftohs(&data[pos + 2]);

    /* find any subscription for that topic */
    mutex_lock(&con->lock);
    asymcute_sub_t *sub = NULL;
    for (asymcute_sub_t *cur = con->subscriptions; cur; cur = cur->next) {
        if (cur->topic->id == topic_id) {
            sub = cur;
            break;
        }
    }

    /* send PUBACK if needed (QoS > 0 or on invalid topic ID) */
    if ((sub == NULL) || (data[pos + 1] & MQTTSN_QOS_1)) {
        uint8_t ret = (sub) ? MQTTSN_ACCEPTED : MQTTSN_REJ_INV_TOPIC_ID;
        uint8_t pkt[7] = { 7, MQTTSN_PUBACK, 0, 0, 0, 0, ret };
        /* copy topic and message id */
        memcpy(&pkt[2], &data[pos + 2], 4);
        send_unicast(con, pkt, 7);
    }

    /* release the context and notify the user (on success) */
    mutex_unlock(&con->lock);
    if (sub) {
        sub->cb(sub, ASYMCUTE_PUBLISHED,
                &data[pos + 6], (len - (pos + 6)), sub->arg);
    }
}

static void _on_puback(asymcute_con_t *con, const uint8_t *data, size_t len)
{
    mutex_lock(&con->lock);
    asymcute_req_t *req = _req_preprocess(con, len, MINLEN_PUBACK,
                                          data, IDPOS_PUBACK);
    /* if not use later by the application, releasing the request
       as it should be done in the _req_preprocess function */
    req->con = NULL;

    if (req == NULL) {
        mutex_unlock(&con->lock);
        return;
    }

    unsigned ret = (data[6] == MQTTSN_ACCEPTED) ?
                    ASYMCUTE_PUBLISHED : ASYMCUTE_REJECTED;
    mutex_unlock(&req->lock);
    mutex_unlock(&con->lock);
    con->user_cb(req, ret);
}

static void _on_suback(asymcute_con_t *con, const uint8_t *data, size_t len)
{
    mutex_lock(&con->lock);
    asymcute_req_t *req = _req_preprocess(con, len, MINLEN_SUBACK,
                                          data, IDPOS_SUBACK);
    /* if not use later by the application, releasing the request
       as it should be done in the _req_preprocess function */
    req->con = NULL;

    if (req == NULL) {
        mutex_unlock(&con->lock);
        return;
    }

    unsigned ret = ASYMCUTE_REJECTED;
    if (data[7] == MQTTSN_ACCEPTED) {
        /* parse and apply assigned topic id */
        asymcute_sub_t *sub = (asymcute_sub_t *)req->arg;
        sub->topic->id = byteorder_bebuftohs(&data[3]);
        sub->topic->con = con;
        /* insert subscription to connection context */
        sub->next = con->subscriptions;
        con->subscriptions = sub;
        ret = ASYMCUTE_SUBSCRIBED;
    }

    /* notify the user */
    mutex_unlock(&req->lock);
    mutex_unlock(&con->lock);
    con->user_cb(req, ret);
}

static void _on_unsuback(asymcute_con_t *con, const uint8_t *data, size_t len)
{
    mutex_lock(&con->lock);
    asymcute_req_t *req = _req_preprocess(con, len, MINLEN_UNSUBACK,
                                          data, IDPOS_UNSUBACK);
    /* if not use later by the application, releasing the request
       as it should be done in the _req_preprocess function */
    req->con = NULL;

    if (req == NULL) {
        mutex_unlock(&con->lock);
        return;
    }

    /* remove subscription from list */
    asymcute_sub_t *sub = (asymcute_sub_t *)req->arg;
    if (con->subscriptions == sub) {
        con->subscriptions = sub->next;
    }
    else {
        for (asymcute_sub_t *e = con->subscriptions; e && e->next; e = e->next) {
            if (e->next == sub) {
                e->next = e->next->next;
                break;
            }
        }
    }

    /* reset subscription context */
    sub->topic = NULL;

    /* notify user */
    mutex_unlock(&req->lock);
    mutex_unlock(&con->lock);
    con->user_cb(req, ASYMCUTE_UNSUBSCRIBED);
}

static void _on_willtopicreq(asymcute_con_t *con, const uint8_t *data, size_t len)
{
    mutex_lock(&con->lock);

    asymcute_req_t *req = _req_preprocess(con, len, MINLEN_WILLTOPICREQ, NULL, 0);
    /* if not use later by the application, releasing the request
       as it should be done in the _req_preprocess function */
    req->con = NULL;

    if (req == NULL) {
        DEBUG("_on_willtopicreq ERROR req = NULL\n");
        mutex_unlock(&con->lock);
        return;
    }

    /* notify user */
    mutex_unlock(&req->lock);
    mutex_unlock(&con->lock);
    con->user_cb(con, ASYMCUTE_WILLTOPIC);
}

static void _on_willmsgreq(asymcute_con_t *con, const uint8_t *data, size_t len)
{
    mutex_lock(&con->lock);

    asymcute_req_t *req = _req_preprocess(con, len, MINLEN_WILLMSGREQ, NULL, 0);
    /* if not use later by the application, releasing the request
       as it should be done in the _req_preprocess function */
    req->con = NULL;

    if (req == NULL) {
        DEBUG("_on_willmsgreq ERROR req = NULL\n");
        mutex_unlock(&con->lock);
        return;
    }

    /* notify user */
    mutex_unlock(&req->lock);
    mutex_unlock(&con->lock);
	con->user_cb(con, ASYMCUTE_WILLMSG);
}

static void msg_handler(void *arg)
{
    asymcute_con_t *con = (asymcute_con_t *)arg;
    size_t len;
    size_t pos = _len_get(con->rxbuf, &len);
    uint8_t type = con->rxbuf[pos];

    DEBUG("\nmsg_handler from asymcute\n");

#if defined(MODULE_D7A) && !defined(MODULE_GNRC_SOCK_UDP)
    /* make sure the incoming data was send by 'our' gateway in case of unicast message*/
    if ((type != MQTTSN_GWINFO) && ( type != MQTTSN_ADVERTISE))
    {

        DEBUG("\nour expected gw\n");
        char *ptr = &con->gateway[con->gw_connected].addr;
        for( uint32_t i=0 ; i<8 ; i++ )
        {
            printf(" %02X", ptr[i]);
        }

        DEBUG("\nthe received address\n");

        ptr = &con->remote_addr;
        for( uint32_t i=0 ; i<8 ; i++ )
        {
            printf(" %02X", ptr[i]);
        }
    	if (memcmp(&con->gateway[con->gw_connected].addr, &con->remote_addr, sizeof(d7a_address))!=0) {
            DEBUG("\nERROR, incoming data not sent by our gateway, discard data\n");
            return;
        }
    }
#endif

    DEBUG("\n Received message type %d \n", type);

    /* figure out required action based on message type */
    switch (type) {
        case MQTTSN_GWINFO:
            _on_gw_info(con, con->rxbuf, len);
            break;
        case MQTTSN_CONNACK:
            _on_connack(con, con->rxbuf, len);
            break;
        case MQTTSN_DISCONNECT:
            _on_disconnect(con, len);
            break;
        case MQTTSN_PINGREQ:
            _on_pingreq(con);
            break;
        case MQTTSN_PINGRESP:
            _on_pingresp(con);
            break;
        case MQTTSN_REGACK:
            _on_regack(con, con->rxbuf, len);
            break;
        case MQTTSN_PUBLISH:
            _on_publish(con, con->rxbuf, pos, len);
            break;
        case MQTTSN_PUBACK:
            _on_puback(con, con->rxbuf, len);
            break;
        case MQTTSN_SUBACK:
            _on_suback(con, con->rxbuf, len);
            break;
        case MQTTSN_UNSUBACK:
            _on_unsuback(con, con->rxbuf, len);
            break;
        case MQTTSN_WILLTOPICREQ:
            _on_willtopicreq(con, con->rxbuf, con->rxlen);
            break;
        case MQTTSN_WILLMSGREQ:
            _on_willmsgreq(con, con->rxbuf, con->rxlen);
            break;
        default:
            break;
    }
}



#if defined(MODULE_D7A) && !defined(MODULE_GNRC_SOCK_UDP)
static void _on_data(void *arg, uint8_t *pkt, uint8_t pkt_len, d7a_address *remote)
{
    asymcute_con_t *con = (asymcute_con_t *)arg;
    size_t len;
    size_t pos = _len_get(pkt, &len);

    memcpy(&con->remote_addr, remote, sizeof(d7a_address));

    /* validate incoming data: verify message length */
    if ((pkt_len < 2) ||
        (pkt_len <= pos) || (pkt_len < len)) {
        /* length field of MQTT-SN packet seems to be invalid -> drop the pkt */
        return;
    }

    //
    memcpy(con->rxbuf, pkt, len);
    con->rxlen = pkt_len;
    event_post(&_queue, &con->d7a_evt.super);
}
#else
static void _on_data(asymcute_con_t *con, size_t pkt_len, sock_udp_ep_t *remote)
{
    size_t len;
    size_t pos = _len_get(con->rxbuf, &len);

    /* make sure the incoming data was send by 'our' gateway */
    if (!sock_udp_ep_equal(&con->server_ep, remote)) {
        return;
    }

    /* validate incoming data: verify message length */
    if ((pkt_len < 2) ||
        (pkt_len <= pos) || (pkt_len < len)) {
        /* length field of MQTT-SN packet seems to be invalid -> drop the pkt */
        return;
    }

    msg_handler(con);
}

void *_listener(void *arg)
{
    asymcute_con_t *con = (asymcute_con_t *)arg;

    /* create a socket for this listener, using an ephemeral port */
    sock_udp_ep_t local = SOCK_IPV6_EP_ANY;
    if (sock_udp_create(&con->sock, &local, NULL, 0) != 0) {
        LOG_ERROR("[asymcute] error creating listener socket\n");
        return NULL;
    }

    while (1) {
        sock_udp_ep_t remote;
        int n = sock_udp_recv(&con->sock, con->rxbuf, ASYMCUTE_BUFSIZE,
                              SOCK_NO_TIMEOUT, &remote);
        if (n > 0) {
            _on_data(con, (size_t)n, &remote);
        }
    }

    /* should never be reached */
    return NULL;
}
#endif

void *_handler(void *arg)
{
    (void)arg;
    event_queue_init(&_queue);
    event_loop(&_queue);
    /* should never be reached */
    return NULL;
}

int asymcute_listener_run(asymcute_con_t *con, char *stack, size_t stacksize,
                          char priority, asymcute_evt_cb_t callback)
{
    /* make sure con is not running */
    assert(con);
    assert((priority > 0) && (priority < THREAD_PRIORITY_IDLE - 1));
    assert(callback);

    int ret = ASYMCUTE_OK;

    /* make sure the connection context is not already used */
    mutex_lock(&con->lock);
    if (con->state != UNINITIALIZED) {
        ret = ASYMCUTE_BUSY;
        goto end;
    }

    /* initialize the connection context */
    memset(con, 0, sizeof(asymcute_con_t));
    random_bytes((uint8_t *)&con->last_id, 2);
    event_callback_init(&con->keepalive_evt, _on_keepalive_evt, con);
    event_timeout_init(&con->keepalive_timer, &_queue, &con->keepalive_evt.super);
    con->keepalive_retry_cnt = ASYMCUTE_N_RETRY;
    con->state = NOTCON;
    con->user_cb = callback;

#if defined(MODULE_D7A) && !defined(MODULE_GNRC_SOCK_UDP)
    d7a_register_receive_callback(_on_data, con);
    event_callback_init(&con->d7a_evt, msg_handler, con);

    // reset GW infos
    con->gw_count = 0;
    con->gw_connected = 0;
#else
    /* start listener thread */
    thread_create(stack,
                  stacksize,
                  priority,
                  THREAD_CREATE_WOUT_YIELD,
                  _listener,
                  con,
                  "asymcute_listener");
#endif

end:
    mutex_unlock(&con->lock);
    return ret;
}

void asymcute_handler_run(void)
{
    thread_create(_stack, sizeof(_stack), ASYMCUTE_HANDLER_PRIO,
                   THREAD_CREATE_STACKTEST, _handler, NULL, "asymcute_main");
}

int asymcute_topic_init(asymcute_topic_t *topic, const char *topic_name,
                        uint16_t topic_id)
{
    assert(topic);

    size_t len = 0;

    if (asymcute_topic_is_reg(topic)) {
        return ASYMCUTE_REGERR;
    }

    if (topic_name == NULL) {
        if ((topic_id == 0) || (topic_id == UINT16_MAX)) {
            return ASYMCUTE_OVERFLOW;
        }
    }
    else {
        len = strlen(topic_name);
        if ((len == 0) || (len > ASYMCUTE_TOPIC_MAXLEN)) {
            return ASYMCUTE_OVERFLOW;
        }
    }

    /* reset given topic */
    asymcute_topic_reset(topic);
    /* pre-defined topic ID? */
    if (topic_name == NULL) {
        topic->id = topic_id;
        topic->flags = MQTTSN_TIT_PREDEF;
        memcpy(topic->name, &topic_id, 2);
        topic->name[2] = '\0';
    }
    else {
        strncpy(topic->name, topic_name, sizeof(topic->name));
        if (len == 2) {
            memcpy(&topic->id, topic_name, 2);
            topic->flags = MQTTSN_TIT_SHORT;
        }
    }

    return ASYMCUTE_OK;
}

bool asymcute_is_connected(const asymcute_con_t *con)
{
    return (con->state == CONNECTED);
}

int asymcute_connect(asymcute_con_t *con, asymcute_req_t *req, const char *cli_id,
                     const char *addr, bool clean, asymcute_will_t *will)
{
    assert(con);
    assert(req);
    assert(cli_id);

    int ret = ASYMCUTE_OK;
    size_t id_len = strlen(cli_id);

    /* make sure the client ID will fit into the dedicated buffer */
    if ((id_len < MQTTSN_CLI_ID_MINLEN) || (id_len > MQTTSN_CLI_ID_MAXLEN)) {
        return ASYMCUTE_OVERFLOW;
    }
    /* check if the context is not already connected to any gateway */
    mutex_lock(&con->lock);
    if (con->state != NOTCON) {
        if(con->state != SLEEP)
        {
            ret = ASYMCUTE_GWERR;
            goto end;
        }
    }
    /* get mutual access to the request context */
    if (mutex_trylock(&req->lock) != 1) {
        ret = ASYMCUTE_BUSY;
        goto end;
    }

    /* build the CONNECT message according the provided settings */
    req->msg_id = 0;
    req->broadcast = false;
    req->data[0] = (uint8_t)(id_len + 6);
    req->data[1] = MQTTSN_CONNECT;
    req->data[2] = ((clean) ? MQTTSN_CS : 0);
    if (will) req->data[2] |= MQTTSN_WILL;
    req->data[3] = PROTOCOL_VERSION;
    byteorder_htobebufs(&req->data[4], ASYMCUTE_KEEPALIVE);
    memcpy(&req->data[6], cli_id, id_len);
    req->data_len = (size_t)req->data[0];

    // store the client Id used to identify the client to the server
    strncpy(con->cli_id, cli_id, sizeof(con->cli_id));

    if ((addr == NULL) || (strlen(addr) == 0))
    {
        // No GW address provided, use the default one or start the discovery procedure
        if (!con->gw_count) // no active gateway
        {
            /* prepare for gateway discovery */
            con->state = SEARCHING_GW;
            con->pending = req;
            _req_send(&search_gw_req, con, _on_con_timeout);
            goto end;
        }
    }
    else
    {
#if defined(MODULE_D7A) && !defined(MODULE_GNRC_SOCK_UDP)
        if (con->gw_count == ASYMCUTE_MAX_ACTIVE_GW)
        // override the last GW address if the list is full
        if (con->gw_count == ASYMCUTE_MAX_ACTIVE_GW)
            con->gw_count--;

        memcpy(&con->gateway[con->gw_count].addr, addr, sizeof(d7a_address));

        // Force to use this GW address
        con->gw_connected = con->gw_count;
        con->gw_count++;
#else
        sock_udp_ep_t ep;
        if (sock_udp_str2ep(&ep, addr) != 0) {
            puts("error: unable to parse gateway address");
            ret = ASYMCUTE_GWERR;
            goto end;
        }

        if (ep.port == 0) {
            ep.port = MQTTSN_DEFAULT_PORT;
        }

        memcpy(&con->server_ep, ep, sizeof(con->server_ep));
#endif
    }

    /* prepare the connection context */
    con->state = CONNECTING;
    _req_send(req, con, _on_con_timeout);

end:
    mutex_unlock(&con->lock);
    return ret;
}

int asymcute_disconnect(asymcute_con_t *con, asymcute_req_t *req, uint16_t duration)
{
    assert(con);
    assert(req);

    int ret = ASYMCUTE_OK;

    /* check if we are actually connected */
    mutex_lock(&con->lock);
    if (!asymcute_is_connected(con)) {
        ret = ASYMCUTE_GWERR;
        goto end;
    }
    /* get mutual access to the request context */
    if (mutex_trylock(&req->lock) != 1) {
        ret = ASYMCUTE_BUSY;
        goto end;
    }

    /* prepare and send disconnect message */
    if (duration == NULL)
    {
        /* put connection into TEARDOWN state */
        _disconnect(con, TEARDOWN);

        req->msg_id = 0;
        req->data[0] = 2;
        req->data[1] = MQTTSN_DISCONNECT;
        req->data_len = 2;
        _req_send(req, con, _on_discon_timeout);
    }
    else
    {
        req->msg_id = 0;
        req->data[0] = 4;
        req->data[1] = MQTTSN_DISCONNECT;
        byteorder_htobebufs(&req->data[2], duration);
        req->data_len = 4;
        con->state = SLEEPING;
        _req_send(req, con, _on_discon_timeout);
    }

end:
    mutex_unlock(&con->lock);
    return ret;
}

int asymcute_register(asymcute_con_t *con, asymcute_req_t *req,
                      asymcute_topic_t *topic)
{
    assert(con);
    assert(req);
    assert(topic);

    int ret = ASYMCUTE_OK;

    /* test if topic is already registered */
    if (asymcute_topic_is_reg(topic)) {
        return ASYMCUTE_REGERR;
    }
    /* make sure we are connected */
    mutex_lock(&con->lock);
    if (!asymcute_is_connected(con)) {
        ret = ASYMCUTE_GWERR;
        goto end;
    }
    /* get mutual access to the request context */
    if (mutex_trylock(&req->lock) != 1) {
        ret = ASYMCUTE_BUSY;
        goto end;
    }

    /* prepare topic */
    req->arg = (void *)topic;
    size_t topic_len = strlen(topic->name);

    /* prepare registration request */
    req->msg_id = _msg_id_next(con);
    size_t pos = _len_set(req->data, (topic_len + 5));
    req->data[pos] = MQTTSN_REGISTER;
    byteorder_htobebufs(&req->data[pos + 1], 0);
    byteorder_htobebufs(&req->data[pos + 3], req->msg_id);
    memcpy(&req->data[pos + 5], topic->name, topic_len);
    req->data_len = (pos + 5 + topic_len);

    /* send the request */
    _req_send(req, con, NULL);

end:
    mutex_unlock(&con->lock);
    return ret;
}

int asymcute_publish(asymcute_con_t *con, asymcute_req_t *req,
                     const asymcute_topic_t *topic,
                     const void *data, size_t data_len, uint8_t flags)
{
    assert(con);
    assert(req);
    assert(topic);
    assert((data_len == 0) || data);

    int ret = ASYMCUTE_OK;

    /* check for valid flags */
    if ((flags & VALID_PUBLISH_FLAGS) != flags) {
        return ASYMCUTE_NOTSUP;
    }
    /* check for message size */
    if ((data_len + 9) > ASYMCUTE_BUFSIZE) {
        return ASYMCUTE_OVERFLOW;
    }
    /* make sure topic is registered */
    if (!asymcute_topic_is_reg(topic) || (topic->con != con)) {
        return ASYMCUTE_REGERR;
    }
    /* check if we are connected to a gateway */
    mutex_lock(&con->lock);
    if (asymcute_is_connected(con) != 1) {
        ret = ASYMCUTE_GWERR;
        goto end;
    }
    /* make sure request context is clear to be used */
    if (mutex_trylock(&req->lock) != 1) {
        ret = ASYMCUTE_BUSY;
        goto end;
    }

    /* get message id */
    req->msg_id = _msg_id_next(con);

    /* assemble message */
    size_t pos = _len_set(req->data, data_len + 6);
    req->data[pos] = MQTTSN_PUBLISH;
    req->data[pos + 1] = (flags | topic->flags);
    byteorder_htobebufs(&req->data[pos + 2], topic->id);
    byteorder_htobebufs(&req->data[pos + 4], req->msg_id);
    memcpy(&req->data[pos + 6], data, data_len);
    req->data_len = (pos + 6 + data_len);

    /* publish selected data */
    if (flags & MQTTSN_QOS_1) {
        _req_send(req, con, NULL);
    }
    else {
        _req_send_once(req, con);
    }

end:
    mutex_unlock(&con->lock);
    return ret;
}

int asymcute_subscribe(asymcute_con_t *con, asymcute_req_t *req,
                       asymcute_sub_t *sub, asymcute_topic_t *topic,
                       asymcute_sub_cb_t callback, void *arg, uint8_t flags)
{
    assert(con);
    assert(req);
    assert(sub);
    assert(topic);
    assert(callback);

    int ret = ASYMCUTE_OK;

    /* check flags for validity */
    if ((flags & VALID_SUBSCRIBE_FLAGS) != flags) {
        return ASYMCUTE_NOTSUP;
    }
    /* is topic initialized? (though it does not need to be registered) */
    if (!asymcute_topic_is_init(topic)) {
        return ASYMCUTE_REGERR;
    }
    /* check if we are connected to a gateway */
    mutex_lock(&con->lock);
    if (!asymcute_is_connected(con)) {
        ret = ASYMCUTE_GWERR;
        goto end;
    }
    /* check if we are already subscribed to the given topic */
    for (asymcute_sub_t *sub = con->subscriptions; sub; sub = sub->next) {
        if (asymcute_topic_equal(topic, sub->topic)) {
            ret = ASYMCUTE_SUBERR;
            goto end;
        }
    }
    /* make sure request context is clear to be used */
    if (mutex_trylock(&req->lock) != 1) {
        ret = ASYMCUTE_BUSY;
        goto end;
    }

    /* prepare subscription context */
    sub->cb = callback;
    sub->arg = arg;
    sub->topic = topic;

    /* send SUBSCRIBE message */
    _compile_sub_unsub(req, con, sub, MQTTSN_SUBSCRIBE);
    _req_send(req, con, _on_suback_timeout);

end:
    mutex_unlock(&con->lock);
    return ret;
}

int asymcute_unsubscribe(asymcute_con_t *con, asymcute_req_t *req,
                         asymcute_sub_t *sub)
{
    assert(con);
    assert(req);
    assert(sub);

    int ret = ASYMCUTE_OK;

    /* make sure the subscription is actually active */
    if (!asymcute_sub_active(sub)) {
        return ASYMCUTE_SUBERR;
    }
    /* check if we are connected to a gateway */
    mutex_lock(&con->lock);
    if (!asymcute_is_connected(con)) {
        ret = ASYMCUTE_GWERR;
        goto end;
    }
    /* make sure request context is clear to be used */
    if (mutex_trylock(&req->lock) != 1) {
        ret = ASYMCUTE_BUSY;
        goto end;
    }

    /* prepare and send UNSUBSCRIBE message */
    _compile_sub_unsub(req, con, sub, MQTTSN_UNSUBSCRIBE);
    _req_send(req, con, NULL);

end:
    mutex_unlock(&con->lock);
    return ret;
}

int asymcute_willtopic(asymcute_con_t *con, asymcute_req_t *req,
                       asymcute_will_t *will, uint8_t flags)
{
    assert(con);
    assert(req);
    assert(will);

    int ret = ASYMCUTE_OK;

    /* check if we are connecting to a gateway */
    mutex_lock(&con->lock);
    if (con->state != CONNECTING) {
        ret = ASYMCUTE_GWERR;
        goto end;
    }
    /* make sure request context is clear to be used */
    if (mutex_trylock(&req->lock) != 1) {
        ret = ASYMCUTE_BUSY;
        goto end;
    }
    /* use only QoS and retain flags */
    flags &= 0x70;

    req->data[0] = (size_t)(strlen(will->topic)+3);
    req->data[1] = MQTTSN_WILLTOPIC;
    req->data[2] = flags;
    memcpy(&req->data[3], will->topic, strlen(will->topic));
    req->data_len = (size_t)req->data[0];
    req->broadcast = false;
    _req_send(req, con, _on_con_timeout);

end:
    mutex_unlock(&con->lock);
    return ret;
}

int asymcute_willmsg(asymcute_con_t *con, asymcute_req_t *req,
                     asymcute_will_t *will)
{
    assert(con);
    assert(req);
    assert(will);

    int ret = ASYMCUTE_OK;

    /* check if we are connecting to a gateway */
    mutex_lock(&con->lock);
    if (con->state != CONNECTING) {
        ret = ASYMCUTE_GWERR;
        goto end;
    }
    /* make sure request context is clear to be used */
    if (mutex_trylock(&req->lock) != 1) {
        ret = ASYMCUTE_BUSY;
        goto end;
    }

    req->data[0] = (size_t)(will->msg_len+2);
    req->data[1] = MQTTSN_WILLMSG;
    memcpy(&req->data[2], will->msg, will->msg_len);
    req->data_len = (size_t)req->data[0];
    req->broadcast = false;
    _req_send(req, con, _on_con_timeout);

end:
    mutex_unlock(&con->lock);
    return ret;
}
