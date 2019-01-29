/*
 * comm_manager.h
 *
 *  Created on: Jan 22, 2019
 *      Author: MSc Patryk Silkowski
 */

#ifndef APP_COMM_MANAGER_H_
#define APP_COMM_MANAGER_H_

/* SDK */
#include "mqttsn_client.h"

/* APP */
#include "comm_utils.h"


#define CONN_MGR_SUCCESS         0


typedef int8_t (*comm_manager_event_cb) (mqttsn_event_t * p_event);


/**@brief Function for initializing the MQTTSN client.
 */
void comm_manager_mqttsn_init(const void * p_transport);

void comm_manager_search_gateway(void);

void comm_manager_connect_to_gateway(void);

void comm_manager_disconnect_from_gateway(void);

void comm_manager_set_evt_gateway_found_cb(comm_manager_event_cb cb);

void comm_manager_set_evt_connected_cb(comm_manager_event_cb cb);

void comm_manager_set_evt_disconnect_permit_cb(comm_manager_event_cb cb);

void comm_manager_set_evt_registered_cb(comm_manager_event_cb cb);

void comm_manager_set_evt_published_cb(comm_manager_event_cb cb);

void comm_manager_set_evt_subscribed_cb(comm_manager_event_cb cb);

void comm_manager_set_evt_unsubscribed_cb(comm_manager_event_cb cb);

void comm_manager_set_evt_received_cb(comm_manager_event_cb cb);

void comm_manager_set_evt_timeout_cb(comm_manager_event_cb cb);

void comm_manager_set_evt_gateway_search_timeout_cb(comm_manager_event_cb cb);

/**@brief Function for register the MQTTSN topic to the gateway.
 */
int8_t comm_manager_topic_register(char * p_topic_name,
                                   uint16_t * msg_id);

/**@brief Function for subscribe to MQTTSN topic.
 */
int8_t comm_manager_topic_subscribe(char * p_topic_name,
                                    uint16_t * msg_id);




#endif /* APP_COMM_MANAGER_H_ */
