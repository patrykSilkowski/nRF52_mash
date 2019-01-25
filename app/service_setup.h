/*
 * service_setup.h
 *
 *  Created on: Jan 22, 2019
 *      Author: patryk
 */

#ifndef APP_SERVICE_SETUP_H_
#define APP_SERVICE_SETUP_H_

#include "service_bsp.h"


#define SERVICE_STR_INFO          "info"
#define SERVICE_STR_TIME          "time"
#define SERVICE_STR_PREC          "prec"
#define SERVICE_STR_ONOFF         "onoff"
#define SERVICE_STR_TEMPHUM       "temphum"
#define SERVICE_STR_CONFIG_SUB    "config/sub"
#define SERVICE_STR_CONFIG_UNSUB  "config/unsub"
#define SERVICE_STR_CONFIG_LIST   "config/list"

#define SERVICE_STR_MAX_LENGTH    13

#define SERVICE_MSG_OFF            "off"
#define SERVICE_MSG_ON             "on"

typedef enum {
    lightbulb = 0,
    wall_switch,
    relay,
    thermometer,
    info_none
} service_info_t;

typedef enum {
    info,
    //time,
    //prec,
    onoff,
    //temphum,
    config_sub,
    config_unsub,
    config_list,
    type_none
} service_type_t;

typedef struct {
    uint16_t topic_id;
    service_type_t type;
    endpoint_t endpoint;
} service_data_t;

#endif /* APP_SERVICE_SETUP_H_ */
