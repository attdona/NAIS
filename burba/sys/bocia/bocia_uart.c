/*
 * Copyright (C) 2017 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "thread.h"
#include "mutex.h"
#include "board.h"
#include "uart_stdio.h"

#include "bocia.h"

#define ENABLE_DEBUG                (1)
#include "debug.h"

#define MAX_BIN_SIZE 512


#define IO_ERROR       -1
#define IO_BUFF_OVFLOW -2

static char _uart_stack[1024 + THREAD_EXTRA_STACKSIZE_PRINTF];

static unsigned char _rxbuff[MAX_BIN_SIZE];

static uint8_t _txbuff[MAX_BIN_SIZE];

static mutex_t _txmutex = MUTEX_INIT;

enum {
    WAIT_START, MSG_TYPE, SLINE, DLINE, FLAGS, LEN, BIN_BLOCK, EXPECT_END
};



void protobuf_to_uart(bocia_channel_t * rd, proto_msg_t type, void* obj) {
    int blen;
    int hlen;

    blen = protobuf_c_message_get_packed_size((const ProtobufCMessage*) obj);
    mutex_lock(&_txmutex);
    hlen = nais_setup_header(type, blen, _txbuff);

    protobuf_c_message_pack((const ProtobufCMessage*) obj, _txbuff + 6);
    DEBUG("sending %d bytes with len [%d]\n", hlen+blen+1, _txbuff[5]);
    uart_stdio_write((const char*) _txbuff, hlen+blen+1);
    mutex_unlock(&_txmutex);

}

/**
 * @brief read a binary protobuf from UART
 *
 * param[in] arg id of thread to deliver messages produced by input receiver
 *
 */
void *serial_loop(void* arg) {
    uint8_t *line_buf_ptr = _rxbuff;
    uint8_t sts = WAIT_START;
    int32_t binlen = 0;
    //proto_msg_t message = M_PROTO_ERROR;
    int c;
    int multiplier = 1;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
    bocia_channel_t rd = {
            .target_pid = (kernel_pid_t)arg,
            .send = protobuf_to_uart
    };
#pragma GCC diagnostic pop

    while (1) {
        if ((line_buf_ptr - _rxbuff) >= MAX_BIN_SIZE) {
            // TODO
            //return IO_BUFF_OVFLOW;
        }

        c = getchar();
        if (c < 0) {
            // TODO
            //return IO_ERROR;
        }

        if (c == RECORD_END) {
            if (sts == EXPECT_END) {
                bocia_unmarshall(&rd, _rxbuff, bocia_input_handler);
                sts = WAIT_START;
                line_buf_ptr = _rxbuff;
                binlen = 0;
                continue;

            }
            else {
                // TODO
            }
        }

        switch (sts) {
            case WAIT_START:
                if (c == RECORD_START) {
                    sts = MSG_TYPE;
                    multiplier = 1;
                }
                else {
                    DEBUG("%c", c);
                }
                break;
            case MSG_TYPE:
                *line_buf_ptr++ = c;
                sts = SLINE;
                break;
            case SLINE:
                *line_buf_ptr++ = c;
                sts = DLINE;
                break;
            case DLINE:
                *line_buf_ptr++ = c;
                sts = FLAGS;
                break;
            case FLAGS:
                *line_buf_ptr++ = c;
                sts = LEN;
                break;
            case LEN:
                binlen += (c & 127) * multiplier;
                multiplier *= 128;
                if((c & 128) == 0) {
                    sts = BIN_BLOCK;
                }
                *line_buf_ptr++ = c;

                break;
            case BIN_BLOCK:
                *line_buf_ptr++ = c;
                if((line_buf_ptr - _rxbuff) == binlen + 5) {
                    sts = EXPECT_END;
                }

                break;
        }
    }

}

void bocia_uart_init(void) {

    kernel_pid_t sid = thread_create(_uart_stack, sizeof(_uart_stack),
    THREAD_PRIORITY_MAIN - 2,
    THREAD_CREATE_STACKTEST, serial_loop, (void *)(int32_t)thread_getpid(), "uart");
    (void) (sid);
}

