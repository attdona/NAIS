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
 * @file
 *
 *
 * @defgroup     nais NAIS
 * @{
 *
 * @brief       NAIS related definitions
 *
 * @author      Attilio Dona' <attilio.dona>
 *
 */

#ifndef SYS_INCLUDE_NAIS_H_
#define SYS_INCLUDE_NAIS_H_

#include "app_messages.h"
#include "kernel_types.h"

#include "messages.pb-c.h"

/**
 * @brief start byte of nais payload
 */
#define RECORD_START (0x1E)

/**
 * @brief end byte of nais payload
 */
#define RECORD_END (0x17)

/**
 * @brief Bocia types (payload byte BOCIA[0])
 */
enum nais_message_type {
    PROFILE_TYPE = 1, ///< set a profile (user/password)
    CONFIG_TYPE,      ///< network setting (topic, host, port)
    ACK_TYPE,         ///< command ack
    SECRET_TYPE,      ///< exchange a secret
    COMMAND_TYPE,     ///< a builtin or a custom command
    EVENT_TYPE        ///< conveys an event
};


/**
 * @brief builtin commands 
 */
enum command_types {
    REBOOT_CMD = 1,   ///< board reboot (soft RESET) 
    TOGGLE_WIFI_MODE_CMD, ///< toggle between STA and AP mode
    FACTORY_RESET_CMD, ///< delete config file and reboot
    OTA_CMD,           ///< Over The Air firmware update
    GET_CONFIG_CMD     ///< return the current board configuration
};


/**
 * @brief NAIS header
 */
typedef struct {
    uint8_t type; ///< NAIS.TYPE field: a unique id that identifies the
                  ///  serialized object contained into NAIS.PAYLOAD

    uint8_t dline; ///< NAIS.DLINE reflected value from NAIS.SLINE request
                   ///  packet that triggers a response. Set to zero if the
                   ///  packet originates from the board.

    uint8_t flags; ///< NAIS.FLAGS field
} proto_msg_t;


/**
 * @brief initialize the bocia packet header
 */
int8_t nais_setup_header(proto_msg_t msg_type, int msg_len, uint8_t *buff);

/**
 * @brief return true if `buff` array is a NAIS payload, false otherwise
 */
int8_t is_nais_payload(unsigned char *buff, int len);

/**
 * @brief serialize a protobuf object
 */
int nais_marshall(proto_msg_t type, void *obj, unsigned char **payload);

/**
 * @brief construct a protobuf object from a NAIS payload
 */
proto_msg_t nais_unmarshall(unsigned char *data, void **obj);

/**
 * @brief free the protobuf `obj` memory
 */
void nais_free(void *obj);

#endif /* SYS_INCLUDE_NAIS_H_ */
/**@}*/
