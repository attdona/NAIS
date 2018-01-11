/*
 * Copyright (C) 2017 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

#include "bocia_mqtt.h"
#include "MQTTPacket.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#if BOCIA_SECURE
#define SL_SSL_CA_CERT_FILE_NAME "/cert/ca.der"
#define SOCK_PROTO SL_SEC_SOCKET

#else
#define SOCK_PROTO 0
#endif

/**
 * maximum bocia payload size
 */
#define MAX_MQTT_PKT_SIZE 256

#define TOPIC_MAX_LEN 64

// the topic prefix
#define TOPIC_PREFIX "hb/"
#define TOPIC_PREFIX_LEN 3

// bocia_connect() set the topic name in this buffer
static char topic_buff[TOPIC_MAX_LEN];

// the mqtt receiver and transmitter buffer
static unsigned char buf[MAX_MQTT_PKT_SIZE];

static bocia_channel_t *mqtt_handler = 0;

static inline int16_t mqtt_close(int16_t fd) {
    sl_Close(fd);
    return 0;
}

int sock_receive(int16_t fd, unsigned char *buf, int count) {
    int rc = sl_Recv(fd, buf, count, 0);

    if (rc == 0) {
        DEBUG("lost broker conn %d, closing\n", fd);
        mqtt_close(fd);
    }

    return rc;
}

int16_t mqtt_receive(int16_t *fdp, unsigned char **packet) {
    int pkt_type;
    pkt_type = MQTTPacket_read(*fdp, buf, MAX_MQTT_PKT_SIZE, sock_receive);
    DEBUG("[%d] sock: recv mqtt packet %d\n", *fdp, pkt_type);
    if (pkt_type == -1) {
        *fdp = 0;
    }

    *packet = buf;
    return pkt_type;
}


void mqtt_receiver_handler(bocia_channel_t *rd) {
    // bocia_receive(rd, bocia_input_handler);
    unsigned char *pkt;
    int16_t type;
    unsigned char dup;
    int qos;
    unsigned char retained;
    unsigned short msgid;
    int payloadlen_in;
    unsigned char *payload_in;
    MQTTString receivedTopic;

    type = mqtt_receive(&rd->fd, &pkt);

    if (rd->fd != 0) {
        // received a valid message

        if (type == PUBLISH) {
            MQTTDeserialize_publish(&dup, &qos, &retained, &msgid,
                                    &receivedTopic, &payload_in, &payloadlen_in,
                                    pkt, MAX_MQTT_PKT_SIZE);

            if (is_nais_payload(payload_in, payloadlen_in)) {
                bocia_unmarshall(rd, ++payload_in, bocia_input_handler);
            }
        }
    }
}

void mqtt_transmitter_handler(bocia_channel_t *rd, proto_msg_t type,
                              void *data) {

    int len;
    unsigned char *payload;

    int payloadlen = nais_marshall(type, data, &payload);

    MQTTString topicString = MQTTString_initializer;

    topicString.cstring = topic_buff;

    len = MQTTSerialize_publish(buf, MAX_MQTT_PKT_SIZE, 0, 1, 0, 0, topicString,
                                payload, payloadlen);

#ifdef DEBUG
    DEBUG("-out-> ");
    for (uint16_t i = 0; i < payloadlen; i++) {
        DEBUG("[%02x] ", payload[i]);
    }
    DEBUG("(%d/%d)\n", payloadlen, len);
#endif
    sl_Send(rd->fd, buf, len, 0);
}

/**
 * @brief initialize a mqtt session, connect to hostname
 *
 * @return the socket id
 */
int16_t mqtt_connect(const char *hostname, uint16_t port, const char *id,
                     short alive_period, char clean) {
    uint32_t ip_server;
    int16_t sock;
    int16_t sts;
    struct SlSockAddrIn_t addr;
    int len;
    MQTTPacket_connectData conn_data = MQTTPacket_connectData_initializer;

#if BOCIA_SUITE
    unsigned char ucMethod = SL_SO_SEC_METHOD_TLSV1_2;
    unsigned int uiCipher = SL_SEC_MASK_TLS_RSA_WITH_AES_256_CBC_SHA256;
#endif

    // sbapi_init(SBAPI_DEFAULT_CFG);
    ip_server = net_atoi(hostname);

    // filling the TCP server socket address
    addr.sin_family = SL_AF_INET;
    addr.sin_port = sl_Htons(port);
    // addr.sin_addr.s_addr = htonl(ip_server);
    addr.sin_addr.s_addr = ip_server;

    sock = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, SOCK_PROTO);
    if (sock < 0) {
        DEBUG("%s:%d sock open failed (reason %d)\n", hostname, port, sock);
        goto exit;
    }
#if BOCIA_SUITE
    //
    // configure the socket as SSLV3.0
    //
    sts = sl_SetSockOpt(sock, SL_SOL_SOCKET, SL_SO_SECMETHOD, &ucMethod,
                        sizeof(ucMethod));
    if (sts < 0) {
        DEBUG("couldn't set socket options\n");
        sock = mqtt_close(sock);
        goto exit;
    }
    //
    // configure the socket as RSA with RC4 128 SHA
    //
    sts = sl_SetSockOpt(sock, SL_SOL_SOCKET, SL_SO_SECURE_MASK, &uiCipher,
                        sizeof(uiCipher));
    if (sts < 0) {
        DEBUG("couldn't set socket options\n");
        sock = mqtt_close(sock);
        goto exit;
    }
#endif

#if BOCIA_SECURE
    //
    // configure the socket with CA certificate - for server verification
    //
    sts = sl_SetSockOpt(sock, SL_SOL_SOCKET, SL_SO_SECURE_FILES_CA_FILE_NAME,
                        SL_SSL_CA_CERT_FILE_NAME,
                        strlen(SL_SSL_CA_CERT_FILE_NAME));

    if (sts < 0) {
        DEBUG("couldn't set socket options\n");
        sock = mqtt_close(sock);
        goto exit;
    }
#endif
#if 0
	sts = sl_SetSockOpt(sock, SL_SOL_SOCKET,
			SO_SECURE_DOMAIN_NAME_VERIFICATION, hostname,
			strlen((const char *) hostname));
	if (sts < 0) {
		DEBUG("couldn't set socket options\n");
		sock = mqtt_close(sock);
		goto exit;
	}
#endif

    len = sizeof(struct SlSockAddrIn_t);

    // connect to the TCP socket to the TCP server address
    sts = sl_Connect(sock, (struct SlSockAddr_t *)&addr, len);
    if (sts < 0) {
        sock = mqtt_close(sock);
        DEBUG("%s:%d sock connect failed (reason %d)\n", hostname, port, sts);
        goto exit;
    }
    DEBUG("connected to broker\n");

    conn_data.clientID.cstring = (char *)id;
    conn_data.keepAliveInterval = alive_period;
    conn_data.cleansession = clean;
    len = MQTTSerialize_connect(buf, MAX_MQTT_PKT_SIZE, &conn_data); /* 1 */
    sl_Send(sock, buf, len, 0);

    /* wait for connack */
    sts = MQTTPacket_read(sock, buf, MAX_MQTT_PKT_SIZE, sock_receive);
    switch (sts) {
        case CONNACK: {
            unsigned char sessionPresent, connack_rc;

            if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf,
                                        MAX_MQTT_PKT_SIZE) != 1 ||
                connack_rc != 0) {
                DEBUG("Unable to connect, return code %d\n", connack_rc);
                sock = mqtt_close(sock);
            }
            break;
        }
        case -1:
            sock = mqtt_close(sock);
            break;
    }

exit:
    return sock;
}

int mqtt_subscribe(int16_t fd, char *topic) {
    int len;

    MQTTString topicString = MQTTString_initializer;
    int msgid = 1;
    int req_qos = 1;

    /* subscribe */
    topicString.cstring = topic;
    len = MQTTSerialize_subscribe(buf, MAX_MQTT_PKT_SIZE, 0, msgid, 1,
                                  &topicString, &req_qos);

    // TODO: check return code
    sl_Send(fd, buf, len, 0);

    return 0;
}

int mqtt_unsubscribe(int16_t fd, char *topic) {
    int len;

    MQTTString topicString = MQTTString_initializer;
    int msgid = 1;

    /* subscribe */
    topicString.cstring = topic;
    len = MQTTSerialize_unsubscribe(buf, MAX_MQTT_PKT_SIZE, 0, msgid, 1,
                                    &topicString);

    // TODO: check return code
    sl_Send(fd, buf, len, 0);

    return 0;
}

bocia_channel_t *bocia_mqtt_init(Config *cfg) {
    int16_t sock;
    int16_t port = 1883;
    int16_t alive_period = 120;
    size_t sz;
    char *ptr;

    if (cfg == 0) {
        cfg = bocia_get_config();
    }
    if (cfg == 0) {
        return 0;
    }

    ptr = topic_buff;
    memcpy(ptr, TOPIC_PREFIX, TOPIC_PREFIX_LEN);
    ptr += TOPIC_PREFIX_LEN;
    sz = strlen(cfg->network);
    memcpy(ptr, cfg->network, sz);
    ptr += sz;
    *ptr = '/';
    ptr++;

    sz = strlen(cfg->board);
    memcpy(ptr, cfg->board, sz);
    ptr += sz;
    *ptr = 0;

    DEBUG("bocia_connect host: %s\n", cfg->host);
    if (mqtt_handler && mqtt_handler->fd > 0) {
        DEBUG("already connected: disconnecting %d\n", mqtt_handler->fd);
        // return mqtt_handler;
        mqtt_disconnect(mqtt_handler->fd);

        // an immediate close when select is active hangs forever
        // broker_disconnect(board_io_pid);
        bocia_close_soon(mqtt_handler);
    }
    if (cfg->has_port) {
        port = cfg->port;
    }
    if (cfg->has_alive_period) {
        alive_period = cfg->alive_period;
    }

    sock = mqtt_connect(cfg->host, port, "me", alive_period, 1);

    if (sock > 0) {
        mqtt_handler = bocia_register_handlers(sock, thread_getpid(),
                                               mqtt_receiver_handler,
                                               mqtt_transmitter_handler);
        mqtt_subscribe(sock, topic_buff + TOPIC_PREFIX_LEN);
    } else {
        mqtt_handler = 0;
    }

    return mqtt_handler;
}

char mqtt_is_up(int16_t fd) { return (fd > 0) ? 1 : 0; }


void mqtt_disconnect(int16_t fd) {
    int len;
    len = MQTTSerialize_disconnect(buf, MAX_MQTT_PKT_SIZE);
    sl_Send(fd, buf, len, 0);
}

