/*
 * Copyright (C) 2017 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 *
 */

/**
 * @file
 *
 * @defgroup     bocia BOCIA
 * @{
 *
 *
 * @brief       the bocia layer
 *
 * @author      Attilio Dona' <attilio.dona>
 *
 *
 */

#ifndef SYS_INCLUDE_BOCIA_H_
#define SYS_INCLUDE_BOCIA_H_

#include "kernel_types.h"
#include "sbapi.h"
#include "stdint.h"

#include "nais.h"
#include "periph/pm.h"

/**
 * @brief EXPERIMENTAL: enable remote debug through stdio redirection to socket 
 */
#define DEBUG_OPTION_BIT 0

/**
 * @brief Config.network value to be set when requesting a direct tcp connection
 *
 * Config.network value to be set when requesting a direct tcp connection.
 * All others values for Config.network imply a mqtt connection
 */
#define BOCIA_NETWORK_FOR_TCP "_tcp_"

/**
 * @brief ssl encrypted (1) or plaintext (0) tcp connection
 */
#define BOCIA_SECURE 0

/**
 * @brief if True set a specific security method and cypher
 *
 * if (BOCIA_SECURE && 1) configure security method TLS v1.2 and cipher
 * TLS_RSA_WITH_AES_256_CBC_SHA256
 *
 * if (BOCIA_SECURE && 0) lets security layer to negotiate the security method
 * and the cipher suite
 */
#define BOCIA_SUITE (BOCIA_SECURE && 1)

/**
 * @brief input handler return codes
 */
enum { INPUTHANDLER_OK = 0, INPUTHANDLER_ERR };

/// the bocia configuration filename
#define BOCIA_CFG_FILE "bocia.cfg"

/**
 * @brief a channel abstraction
 */
typedef struct bocia_channel_t {
    kernel_pid_t target_pid; ///< reader thread

    int16_t fd; ///< socket file descriptor

    void (*handler)(struct bocia_channel_t *); ///< receive handler callback

    void (*send)(struct bocia_channel_t *, proto_msg_t, void *proto_obj);
    ///< transmit handler function. Send protobuf object pointed by
    ///``proto_obj``

    int8_t close_pending; ///< a close request is pending

    int8_t debug : 1; ///< channel set in debug mode (EXPERIMENTAL FEATURE)

} bocia_channel_t;

#define SBAPI_SOCKETS 8 /**< max numbers of sockets */


// app_state sockloop_status(void);

/**
 * Unique socket identifier
 */
typedef int16_t reader_id_t;

/**
 * @brief define the custom command logics
 */
uint32_t __attribute__((weak)) handle_command(uint8_t type, Command *command);

/**
 * @brief Connect to a tcp server or a mqtt broker
 * 
 * Connect to a tcp server only if Config.network equals "_tcp_",
 * otherwise a mqtt connection is attempted.
 * 
 * @param[in] cfg a bocia configuration or 0 for default
 * @return the connection handler
 */
bocia_channel_t *bocia_init(Config* cfg);

/**
 * @brief close as soon as possible the mqtt socket
 * 
 * close as soon as possible the channel:
 * this API is needed because cc3200 sl_Select hangs up if a channel's socket
 * is closed meanwhile sl_Select is waiting
 *
 */
void bocia_close_soon(bocia_channel_t *channel);

/**
 * @brief start the event loop
 */
void bocia_loop_init(void);

/**
 * @brief register read and write handlers for a channel
 * 
 * register read and write handlers for a channel
 * 
 * @param[in] sock socket file descriptor
 * @param[in] thread_id channel events (currently `CLOSE` event) are notified
 *                      to `thread_id`
 * @param[in] handler read callback, invoked when bytes are received
 * @param[in] tx_handler write function used to send bytes
 * 
 * @return a channel_reader pointer or NULL if channel resources are unavailable  
 */
bocia_channel_t *bocia_register_handlers(
    int16_t sock, kernel_pid_t thread_id, void (*handler)(bocia_channel_t *),
    void (*tx_handler)(struct bocia_channel_t *, proto_msg_t, void *data));


/**
 * @brief save on File System the bocia configuration
 */
int bocia_write_cfg(Config *cfg);

/**
 * @brief read from File System the bocia configuration
 */
Config *bocia_read_cfg(void);

/**
 * @brief delete a file, mainly used for testing
 */
int16_t bocia_delete_file(const char *fname);


/**
 * @brief broadcast a protobuf message through all opened channels
 *
 */
void bocia_broadcast(proto_msg_t type, void *data);

/**
 * @brief return true if some socket channels are in debug mode
 */
uint8_t bocia_remote_console(void);

/**
 * @brief write to a debug enabled socket channel
 */
_ssize_t bocia_remote_write(const void *data, size_t count);

/**
 * @brief bocia message received callback
 *
 * Each time a bocia message is received bocia_input_handler is invoked
 * with @p obj set to the unmarshaled protobuf object received.
 *
 * @param[in] rd input channel descriptor
 * @param[in] pkt_type packet type
 * @param[in] obj protobuf object
 *
 * @return INPUTHANDLER_OK if callback evaluation is successful,
 *         INPUTHANDLER_ERR otherwise
 *
 */
int8_t bocia_input_handler(bocia_channel_t *rd, proto_msg_t pkt_type,
                           void *obj);

/**
 * @brief unmarshall a NAIS payload, build a protobuf and call back the handler
 *
 * @return INPUTHANDLER_OK if unmarshalled successfully or INPUTHANDLER_ERR
 *         otherwise
 *
 */
int8_t bocia_unmarshall(bocia_channel_t *rd, unsigned char *data,
                        int8_t (*handler)(bocia_channel_t *, proto_msg_t,
                                          void *));

/**
 * @brief return board configuration setting stored on flash memory
 *
 * @return config pointer object or zero if bocia.cfg file not found on flash
 */
Config *bocia_get_config(void);

/**
 * @brief store to File System @p new_config bocia configuration
 */
void bocia_set_cfg(Config *new_config);

/**
 * @brief toggle between STA and AP station mode
 */
Config *bocia_toggle_board_mode(void);

/**
 * @brief execute a standard bocia command
 */
void bocia_command(bocia_channel_t *rd, Command *cmd);

/**@}*/

#endif /* SYS_INCLUDE_BOCIA_H_ */
