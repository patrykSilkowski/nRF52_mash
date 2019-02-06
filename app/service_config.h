/*
 * service_config.h
 *
 *  Created on: Jan 29, 2019
 *      Author: MSc Patryk Silkowski
 */

#ifndef APP_SERVICE_CONFIG_H_
#define APP_SERVICE_CONFIG_H_


/* GCC */
#include <stdint.h>

/* APP */
#include "service_bsp.h"
#include "service_setup.h"


int8_t service_config_subscribe(endpoint_t endpoint,
                                uint8_t * p_msg,
                                uint16_t msg_length);

int8_t service_config_add_ext_topic(uint16_t ret_msg_id, uint16_t topic_id);

#endif /* APP_SERVICE_CONFIG_H_ */
