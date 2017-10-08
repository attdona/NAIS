/*
 * Copyright (C) 2017 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */
/**
 *
 * @addtogroup  bocia
 * @{
 *
 * @file
 * @brief       bocia tcp adaptation layer
 *
 * @author      Attilio Dona' <attilio.dona>
 *
 * @}
 */


#include <xtimer.h>

#include "sbapi.h"
#include "bocia.h"

#define ENABLE_DEBUG    (1)
#include "debug.h"


#define MAX_BUFF_SIZE 256

static unsigned char buff[MAX_BUFF_SIZE];

static int16_t server_fd = 0;

void uart_remote(const uint8_t *data, size_t len) {

}

int16_t bocia_create_tcp_server(uint16_t port) {
    SlSockAddrIn_t addr;
    int iAddrSize;
    int16_t fd;
    int sts;

    //filling the TCP server socket address
    addr.sin_family = SL_AF_INET;
    addr.sin_port = sl_Htons(port);
    addr.sin_addr.s_addr = 0;

    fd = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
    if(fd < 0) {
        return fd;
    }
    iAddrSize = sizeof(SlSockAddrIn_t);

    // binding the TCP socket to the TCP server address
    sts = sl_Bind(fd, (SlSockAddr_t *) &addr, iAddrSize);
    if(sts < 0) {
        sl_Close(fd);
        return sts;
    }
    // putting the socket for listening to the incoming TCP connection
    sts = sl_Listen(fd, 0);
    if(sts < 0) {
        sl_Close(fd);
        return sts;
    }

    return fd;
}

void bocia_destroy_tcp_server(void) {
    sl_Close(server_fd);
    server_fd = 0;
}

void bocia_tcp_receive(bocia_channel_t *ch) {
    int rc = 0;
    rc = sl_Recv(ch->fd, buff, MAX_BUFF_SIZE, 0);

#ifdef DEBUG
    DEBUG("bocia_tcp_receive (ch:%d): %d bytes\n", ch->fd, rc);
    for(uint16_t i = 0; i < rc; i++) {
        DEBUG("[%x] ", buff[i]);
    }

    DEBUG("\n");
#endif

    if(rc > 0) {
        buff[rc] = 0;
        bocia_unmarshall(ch, buff+1, bocia_input_handler);
    }
    else if(rc == 0) {
        // connection closed
        sl_Close(ch->fd);
        ch->fd = 0;
    }
}

void bocia_tcp_send(struct bocia_channel_t* hd, proto_msg_t type, void *data) {
    int len;
    int16_t sts;

	unsigned char *payload;

	len = nais_marshall(type, data, &payload);

	sts = sl_Send(hd->fd, payload, len, 0);
    DEBUG("bocia_tcp_send (sts:%d, ch:%d): %d bytes\n", sts, hd->fd, len);
}

void bocia_tcp_accept(bocia_channel_t *ch) {
    int16_t sock = SL_SOC_ERROR;
    SlSockAddrIn_t sAddr;
    int iAddrSize = sizeof(SlSockAddrIn_t);

    sock = sl_Accept(ch->fd, (struct SlSockAddr_t *) &sAddr,
                     (SlSocklen_t*) &iAddrSize);

    if(sock > 0) {
        bocia_register_handlers(sock, ch->target_pid, bocia_tcp_receive, bocia_tcp_send);
    }
    else {
        DEBUG("bocia_tcp_accept failed: errcode %d\n", sock);
    }
}

int16_t bocia_tcp_server(uint16_t port, kernel_pid_t *target_pid) {
    if(!server_fd) {
        server_fd = bocia_create_tcp_server(port);
        if(server_fd < 0) {
            return server_fd;
        }
    }

    // no tx handler needed
    bocia_register_handlers(server_fd, *target_pid, bocia_tcp_accept, NULL);

    return 0;

}


int16_t bocia_tcp_connect(const char *hostname, uint16_t server_port) {

    SlSockAddrIn_t addr;
    int iAddrSize;
    int16_t fd;
    int sts;
    uint32_t server_ip;
	DEBUG("bocia_tcp_connect %s:%d\n", hostname, server_port);

    /* Resolve HOST NAME/IP */
    sts = sl_NetAppDnsGetHostByName((signed char *)hostname,
                                          strlen((const char *)hostname),
                                          &server_ip,SL_AF_INET);

    addr.sin_family = SL_AF_INET;
    addr.sin_port = sl_Htons(server_port);
    addr.sin_addr.s_addr = sl_Htonl(server_ip);;

    // create the TCP socket
    fd = sl_Socket(SL_AF_INET, SL_SOCK_STREAM, 0);
    if (fd < 0) {
        return fd;
    }

    iAddrSize = sizeof(SlSockAddrIn_t);

    // connect to the TCP server address
    sts = sl_Connect(fd, (SlSockAddr_t *) &addr, iAddrSize);
    if (sts < 0) {
        sl_Close(fd);
        return sts;
    }

    return fd;
}

bocia_channel_t *bocia_tcp_init(Config* cfg) {
	int16_t fd;

	if (cfg == 0) {
		cfg = bocia_get_config();
	}
	if (cfg == 0 || cfg->host == NULL) {
		return 0;
	}

	fd = bocia_tcp_connect(cfg->host, cfg->port);

	if (fd < 0) {
		return 0;
	} else {
		return bocia_register_handlers(fd, thread_getpid(), bocia_tcp_receive,
				bocia_tcp_send);
	}

}
