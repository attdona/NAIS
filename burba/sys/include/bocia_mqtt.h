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
 * @addtogroup  bocia
 * @{
 *
 *
 * @author      Attilio Dona' <attilio.dona>
 *
 *
 */

#ifndef SYS_INCLUDE_MQTT_H_
#define SYS_INCLUDE_MQTT_H_

#include "bocia.h"

/**
 * @brief connect to the broker
 *
 * @param[in] cfg broker settings: url, secure mode, ...
 *            if cfg==NULL read the stored configuration from file system
 *
 * @return a connection handler or zero if the connection fails
 *
 */
bocia_channel_t *bocia_mqtt_init(Config* cfg);

/**
 *  @brief disconnect cleanly from the mqtt broker
 */
void mqtt_disconnect(int16_t fd);


#endif /* SYS_INCLUDE_MQTT_H_ */
/** @} */