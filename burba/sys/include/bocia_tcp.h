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
 * @addtogroup  bocia
 * @{
 *
  *
 * @author      Attilio Dona' <attilio.dona>
 *
 */

#ifndef SYS_INCLUDE_BOCIA_TCP_H_
#define SYS_INCLUDE_BOCIA_TCP_H_

/**
 * @brief connect to a tcp server
 * @param[in] cfg a bocia configuration or 0 for default
 * @return the connection handler
 */
bocia_channel_t *bocia_tcp_init(Config* cfg);


#endif /* SYS_INCLUDE_BOCIA_TCP_H_ */
/** @} */