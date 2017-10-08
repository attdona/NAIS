/*
 * Copyright (C) 2017 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

#include "bocia_uart.h"
#include "bocia_mqtt.h"
#include "periph/gpio.h"
#include "xtimer.h"
#include <stdio.h>

#define ENABLE_DEBUG (1)
#include "debug.h"

#define LEDS_COMMAND 10

// bit associated with led in Command ivals registers and in related Ack status
#define LED_RED_BIT 0
#define LED_YELLOW_BIT 1
#define LED_GREEN_BIT 2

#define RETRY_TO_CONNECT 9000

struct timer_msg {
    xtimer_t timer;
    uint32_t interval;
    char *text;
    msg_t msg;
};

struct timer_msg msg_a = {.interval = 2 * (1000000),
                          .text = "Hello World",
                          .msg = {.type = RETRY_TO_CONNECT}};

uint32_t handle_command(uint8_t type, Command *command) {
    uint32_t sts = -1;
    switch (type) {
        case LEDS_COMMAND: {
            int32_t mask, values;
            if (command->n_ivals == 2) {
                mask = command->ivals[0];
                values = command->ivals[1];

                if (mask & 1 << LED_RED_BIT) {
                    if (values & 1 << LED_RED_BIT) {
                        gpio_set(LED_RED);
                    } else {
                        gpio_clear(LED_RED);
                    }
                }
                if (mask & 1 << LED_YELLOW_BIT) {
                    if (values & 1 << LED_YELLOW_BIT) {
                        gpio_set(LED_YELLOW);
                    } else {
                        gpio_clear(LED_YELLOW);
                    }
                }
                if (mask & 1 << LED_GREEN_BIT) {
                    if (values & 1 << LED_GREEN_BIT) {
                        gpio_set(LED_GREEN);
                    } else {
                        gpio_clear(LED_GREEN);
                    }
                }
            }
            sts = gpio_read(LED_RED) | gpio_read(LED_YELLOW) << LED_YELLOW_BIT |
                  gpio_read(LED_GREEN) << LED_GREEN_BIT;

            break;
        }
    }

    return sts;
}


void connection_handler(void) {
    DEBUG("try to connect ...\n");
    if (!bocia_mqtt_init(0)) {
        DEBUG("connection failed\n");
        xtimer_set_msg(&msg_a.timer, msg_a.interval, &msg_a.msg,
                       thread_getpid());
    }
    // test event
    printf("board ready\n");
}

int main(void) {

    msg_t msg;

    msg_t msg_queue[SBAPI_MSG_QUEUE_SIZE];

    /* initialize message queue */
    msg_init_queue(msg_queue, SBAPI_MSG_QUEUE_SIZE);

    // start uart monitor
    bocia_uart_init();

    // start WLAN link
    sbapi_init(SBAPI_DEFAULT_CFG);

    // board in Station mode
    sbapi_set_mode(ROLE_STA);

    DEBUG("board init\n");
    while (1) {
        msg_receive(&msg);
        switch (msg.type) {
            case SBAPI_IP_ACQUIRED:

                connection_handler();

                break;

            case RETRY_TO_CONNECT:
                connection_handler();
                break;

            case BOCIA_SOCK_CLOSED:
                DEBUG("lost connection: server socket closed\n");
                // try to reconnect
                connection_handler();
                break;

            default:
                DEBUG("unexpected message %d\n", msg.type);
        }
    }

    return 0;
}
