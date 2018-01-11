/*
 * Copyright (C) 2017 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

#include <xtimer.h>

#include "bocia_mqtt.h"
#include "bocia_tcp.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/**
 * @brief return the channels list
 */
bocia_channel_t *channels(void);

uint8_t broker_is_connected(void);

void broker_disconnect(kernel_pid_t board_io_pid);

/**
 * @brief set how the board will reboot
 *
 * A default reset is triggered as usual by reboot(). If you want to reset the
 * board and perform a factory reset you have to call set_reboot_mode with mode
 * set to 1.
 *
 * If you want switch between STA and AP wifi mode invoke set_reboot_mode with 0
 *
 * @param[in] mode: if 0 it switches between STA and AP,
 *                  if 1 it does a factory reset
 */
void set_reboot_mode(int mode);


static Config *config = 0;

int8_t bocia_input_handler(bocia_channel_t *rd, proto_msg_t command_id,
                           void *obj) {

    proto_msg_t ack_type = {ACK_TYPE, 1, 0};
    ack_type.dline = command_id.dline;

    Ack ack = ACK__INIT;

    ack.id = command_id.type;

    DEBUG("executing [%d] message\n", command_id.type);

    switch (command_id.type) {
        case COMMAND_TYPE: {
            Command *cmd = (Command *)obj;
            DEBUG("command: %ld, seq: %ld\n", cmd->id, cmd->seq);
            bocia_command(rd, cmd);
            break;
        }
        case SECRET_TYPE: {
            Secret *secret = (Secret *)obj;
            set_ap_password(secret->key);
            cc3200_reset();
            break;
        }
        case CONFIG_TYPE: {
            bocia_set_cfg((Config *)obj);
            rd->send(rd, ack_type, &ack);
            break;
        }
        case PROFILE_TYPE: {
            Profile *profile;
            profile = (Profile *)obj;
            DEBUG("ssid: %s - pwd: %s\n", profile->uid, profile->pwd);
            sbapi_add_profile(profile->uid, profile->pwd);
            rd->send(rd, ack_type, &ack);
            break;
        }
        default:
            DEBUG("custom command [%d]\n", command_id.type);
            ack.status = handle_command(command_id.type, (Command *)obj);
            ack.has_status = 1;
            rd->send(rd, ack_type, &ack);
    }
    return INPUTHANDLER_OK;
}

bocia_channel_t *bocia_init(Config* cfg) {
	bocia_channel_t *channel = 0;

	if (cfg == 0) {
		cfg = bocia_get_config();
	}
	if (cfg == 0 || cfg->host == NULL) {
		return channel;
	}

	if (strcmp(cfg->network, BOCIA_NETWORK_FOR_TCP) == 0) {
		channel = bocia_tcp_init(0);
	} else {
		channel = bocia_mqtt_init(0);
	}

	return channel;
}

uint8_t bocia_remote_console(void) {
    bocia_channel_t *channel;

    channel = channels();

    for (uint8_t i = 0; i < SBAPI_SOCKETS; i++) {
        if (channel->debug == 1) {
            return 1;
        }
    }
    return 0;
}

_ssize_t bocia_remote_write(const void *data, size_t count) {
    bocia_channel_t *channel;
    channel = channels();

    for (uint8_t i = 0; i < SBAPI_SOCKETS; i++) {
        if (channel->debug == 1) {
            sl_Send(channel->fd, data, count, 0);
        }
        channel++;
    }

    return count;
}

void bocia_broadcast(proto_msg_t type, void *data) {
    bocia_channel_t *channel;
    channel = channels();

    for (uint8_t i = 0; i < SBAPI_SOCKETS; i++) {
        if (channel->send) {
            channel->send(channel, type, data);
        }
        channel++;
    }
}

int8_t bocia_unmarshall(bocia_channel_t *rd, unsigned char *data,
                        int8_t (*handler)(bocia_channel_t *, proto_msg_t,
                                          void *)) {
    void *obj;
    proto_msg_t ptype;
    int8_t sts;


    ptype = nais_unmarshall(data, &obj);

    if (obj == 0) {
        return INPUTHANDLER_ERR;
    }

    if (ptype.flags & (1 << DEBUG_OPTION_BIT)) {
        rd->debug = 1;
    }

    sts = handler(rd, ptype, obj);

    nais_free(obj);

    return sts;
}

void set_reboot_mode(int mode) {
    if (mode == 1) {
        DEBUG("reset to factory state\n");
        simplelink_to_default_state();
        bocia_delete_file(BOCIA_CFG_FILE);
        PRCMOCRRegisterWrite(0, 0);
    } else {
        PRCMOCRRegisterWrite(0, 1);
    }
}

void bocia_set_cfg(Config *new_config) {
    if (config) {
        nais_free(config);
    }
    bocia_write_cfg(new_config);
	config = 0;
}

Config *bocia_get_config(void) {
    if (config) {
        return config;
    }
    config = bocia_read_cfg();
    return config;
}

Config *bocia_toggle_board_mode(void) {

    int16_t nprofiles;

    nprofiles = sbapi_get_profiles();

    // check if provisioned
    config = bocia_get_config();
    if (config && (nwp.role == ROLE_AP) && nprofiles > 0) {
        sbapi_set_mode(ROLE_STA);
    } else if ((config == 0) && (nwp.role == ROLE_STA)) {
        sbapi_set_mode(ROLE_AP);
    }

    return config;
}

void delayed_reboot(bocia_channel_t *rd) {
    msg_t msg;

    msg.type = REBOOT_REQUEST;
    msg_send(&msg, rd->target_pid);
}

void bocia_command(bocia_channel_t *rd, Command *cmd) {
    switch (cmd->id) {
        case REBOOT_CMD:
            delayed_reboot(rd);
            break;
        case TOGGLE_WIFI_MODE_CMD:
            set_reboot_mode(0);
            delayed_reboot(rd);
            break;
        case FACTORY_RESET_CMD:
            set_reboot_mode(1);
            delayed_reboot(rd);
            break;
        case GET_CONFIG_CMD: {
            proto_msg_t type = {CONFIG_TYPE, 0, 0};

            if (!config) {
                config = bocia_get_config();
            }
            rd->send(rd, type, config); // TODO check null pointer arg
            break;
        }
        default:
            DEBUG("invalid command id: %ld\n", cmd->id);
    }
}
