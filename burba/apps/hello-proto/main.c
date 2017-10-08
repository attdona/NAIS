/*
 * Copyright (C) 2017 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

 /**
  * A contrieved example that initialize only a UART channel. Just for 
  * illustrating the NAIS protobuf interface on the embedded side.
  */

#include "bocia_uart.h"
#include "periph/gpio.h"
#include <stdio.h>

#define ENABLE_DEBUG (1)
#include "debug.h"

#define LEDS_COMMAND 10

// bit associated with led in Command ivals registers and in related Ack status
#define LED_RED_BIT 0
#define LED_YELLOW_BIT 1
#define LED_GREEN_BIT 2

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

int main(void) {
    // start uart monitor
    bocia_uart_init();

    return 0;
}
