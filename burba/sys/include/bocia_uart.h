/*
 * Copyright (C) 2016 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */


/**
 *
 * @file
 *
 *
 * @addtogroup  bocia
 * @{
 *
 * @brief
 *
 * @author      Attilio Dona' <attilio.dona>
 *
 */

#ifndef SYS_INCLUDE_BOCIA_UART_H_
#define SYS_INCLUDE_BOCIA_UART_H_

#include "bocia.h"


/**
 * @brief initialize the serial thread that will receives protobufs from uart
 *
 */
void bocia_uart_init(void);


/**
 * @brief send a protobuffer on serial uart
 *
 * @param[in] channel channel associated with UART endpoint   
 * @param[in] type protobuf message
 * @param[in] obj pointer to a protobuf instance
 *
 */
void protobuf_to_uart(bocia_channel_t *channel, proto_msg_t type, void* obj);

/**@}*/

#endif /* SYS_INCLUDE_BOCIA_UART_H_ */

