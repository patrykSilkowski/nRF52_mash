/*
 * service_setup.h
 *
 *  Created on: Jan 22, 2019
 *      Author: MSc Patryk Silkowski
 */

#ifndef APP_SERVICE_SETUP_H_
#define APP_SERVICE_SETUP_H_

#include "service_bsp.h"

/* GCC */
#include <stdint.h>
#include <stdbool.h>


#define BASE64_LENGTH                12

#define SERVICE_STR_MAX_LENGTH       13
#define SERVICE_ENDPOINT_MAX         9

/*
 * The easiest way to distinguish topics between self and external is the
 * topic ID which are returned by the broker and each one is assigned only
 * for the particular client (they are not broker wide)
 *
 * Endpoints multiplied by services indicates the max self topic ID
 * Atm. 8 * 5 = 40
 */
#define SERVICE_SELF_TOPIC_ID_MAX    40

#define SERVICE_RETRY_CNT_MAX_FLAG   (-8)
#define SERVICE_ALL_REGISTERED_FLAG  (-9)

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

/*
 * There is an assumption that the number of endpoints is in range 0-9
 */
typedef uint8_t endpoint_t;

typedef struct {
    uint16_t topic_id;
    service_type_t type;
    endpoint_t endpoint;
} service_data_t;


int8_t create_self_services_init(void);

/*
 * Returns SERVICE_ALL_REGISTERED_FLAG if finished
 */
int8_t create_self_services_continue(void);

int8_t service_create(char * p_base_id,
                      endpoint_t endpoint,
                      service_type_t type);

bool service_is_created(uint16_t * msg_id);

void service_destroy(void);

int8_t service_register(void);
int8_t service_subscribe(void);

/*
 * Returns SERVICE_RETRY_CNT_MAX_FLAG if retry counter
 * reached DEFAULT_RETRANSMISSION_CNT
 */
int8_t service_retry_subscribe(uint16_t msg_id);
int8_t service_retry_register(uint16_t msg_id);

int8_t service_subscribe_to_registered(uint16_t msg_id, uint16_t topic_id);
int8_t service_insert_to_database(uint16_t msg_id, uint16_t topic_id);

service_data_t * service_pop_with_topic_id(uint16_t topic_id);

#endif /* APP_SERVICE_SETUP_H_ */
