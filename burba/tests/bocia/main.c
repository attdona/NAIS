#include "bocia_uart.h"
#include "unity.h"
#include "xtimer.h"

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
    msg_t msg;
};

struct timer_msg retry = {.interval = 2 * (1000000),
                          .msg = {.type = RETRY_TO_CONNECT}};

int8_t run_test;

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

void app_init(void) {
    if (!bocia_init(0)) {
        DEBUG("connection failed, scheduling retry ...\n");
        xtimer_set_msg(&retry.timer, retry.interval, &retry.msg,
                       thread_getpid());
    }
}

int main(void) {
    msg_t msg;
    msg_t msg_queue[SBAPI_MSG_QUEUE_SIZE];

    /* initialize message queue */
    msg_init_queue(msg_queue, SBAPI_MSG_QUEUE_SIZE);

    // start uart monitor
    bocia_uart_init();
    bocia_delete_file(BOCIA_CFG_FILE);
    // test event
    printf("board ready\n");

    sbapi_init(SBAPI_DEFAULT_CFG);
    sbapi_set_mode(ROLE_STA);

    run_test = 1;
    while (run_test) {
        msg_receive(&msg);
        switch (msg.type) {
            case SBAPI_IP_ACQUIRED:
                // test event
                printf("IP acquired\n");

                app_init();

                break;
            case BOCIA_CONFIG_OK:
                DEBUG("configuration success\n");
                app_init();
        }
        break;
    }

    return 0;
}
