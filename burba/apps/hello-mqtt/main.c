
/*
 * Copyright (C) 2017 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

#include "bocia_mqtt.h"
#include "bocia_tcp.h"
#include "bocia_uart.h"
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

// message types used in this demo
#define RETRY_TO_CONNECT 9000
#define BUTTON_PRESSED 9001

struct timer_msg {
    xtimer_t timer;
    uint32_t interval;
    msg_t msg;
};

struct timer_msg retry = {.interval = 2 * (1000000),
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

bocia_channel_t *app_init(void) {
    bocia_channel_t *channel = bocia_init(0);

    if (!channel) {
        DEBUG("connection failed, scheduling retry ...\n");
        xtimer_set_msg(&retry.timer, retry.interval, &retry.msg,
                       thread_getpid());
    }
    return channel;
}

static xtimer_ticks64_t previous_event_time = {0};

static kernel_pid_t main_pid;

void button_sw3_pressed(void *arg) {
    msg_t msg;
    xtimer_ticks64_t curr_time = xtimer_now64();

    // 200 millisecond debounce period
    if ((curr_time.ticks64 - previous_event_time.ticks64) > 16000000) {
        msg.type = BUTTON_PRESSED;
        msg_send(&msg, main_pid);
    }
    previous_event_time = curr_time;
}

int main(void) {
    bocia_channel_t *channel = 0;
    msg_t msg;
    msg_t msg_queue[SBAPI_MSG_QUEUE_SIZE];

    main_pid = thread_getpid();

    gpio_init_int(PUSH1, GPIO_IN, GPIO_FALLING, button_sw3_pressed, NULL);

    /* initialize message queue */
    msg_init_queue(msg_queue, SBAPI_MSG_QUEUE_SIZE);

    // start uart monitor
    bocia_uart_init();

    // start WLAN link
    sbapi_init(SBAPI_DEFAULT_CFG);

    // set this board in Station mode
    sbapi_set_mode(ROLE_STA);

    while (1) {
        msg_receive(&msg);
        switch (msg.type) {
            case SBAPI_IP_ACQUIRED:
                printf("IP up, starting tcp client ...\n");

                // run the client
                channel = app_init();

                break;
            case BOCIA_SOCK_CLOSED:
            case RETRY_TO_CONNECT:
                // connection failed or socket closed by peer: try to reconnect
                channel = app_init();
                break;
            case BUTTON_PRESSED: {
                proto_msg_t ev_type = {EVENT_TYPE};
                Event event = EVENT__INIT;
                event.id = BUTTON_PRESSED;
                if (channel) {
                    channel->send(channel, ev_type, &event);
                }
                break;
            }
            default:
                DEBUG("unexpected message %d\n", msg.type);
        }
    }

    return 0;
}
