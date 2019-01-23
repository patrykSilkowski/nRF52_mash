/*
 * service_setup.c
 *
 *  Created on: Jan 22, 2019
 *      Author: MSc Patryk Silkowski
 */


#include <string.h>
#include <stdio.h>


#include "service_setup.h"

#include "comm_manager.h"
#include "comm_utils.h"


#define SERVICE_DATA_ARRAY_SIZE     60
#define SERVICE_CREATE_BUFFER_SIZE  4
#define BASE64_LENGTH               12

#define MQTTSN_TOPIC_NAME_LENGTH    32


typedef struct {
    service_data_t  service;
    uint16_t        message_id;
    uint8_t         topic_name[MQTTSN_TOPIC_NAME_LENGTH];
} create_service_t;


static service_data_t service_database[SERVICE_DATA_ARRAY_SIZE];


static endpoint_t       m_iter_endpoints  =  0;
static service_type_t   m_iter_services   =  0;

static uint16_t         m_msg_id          =  0;

int8_t mash_topic_name_serial(uint8_t * id, create_service_t * dataset)
{
    uint8_t str_service_type[SERVICE_STR_MAX_LENGTH] = {0};

    switch (dataset->service.type)
    {
        case info:
            str_service_type = SERVICE_STR_INFO;
        break;

        case onoff:
            str_service_type = SERVICE_STR_ONOFF;
        break;

        case config_sub:
            str_service_type = SERVICE_STR_CONFIG_SUB;
        break;

        case config_unsub:
            str_service_type = SERVICE_STR_CONFIG_UNSUB;
        break;

        case config_list:
            str_service_type = SERVICE_STR_CONFIG_LIST;
        break;

        case type_none:
        default:
            return -1;
        break;
    } // end of switch

    int8_t ret = (int8_t) snprintf(dataset->topic_name,
                                   MQTTSN_TOPIC_NAME_LENGTH,
                                   "%s//%d//%s",
                                   id,
                                   (uint8_t) dataset->service.endpoint,
                                   str_service_type);

    if (ret >= MQTTSN_TOPIC_NAME_LENGTH)
        return -1;
    else
        return 0;
}



static struct
{
    create_service_t * array[SERVICE_CREATE_BUFFER_SIZE];
    uint8_t cnt;
} service_bucket_s = {
    .array = {0},
    .cnt = 0,
};

int8_t service_bucket_push(create_service_t * p_data)
{
    if (service_bucket_s.cnt == SERVICE_CREATE_BUFFER_SIZE)
        return -1;  // no room

    for (int8_t i = 0; i < SERVICE_CREATE_BUFFER_SIZE; i++)
    {
        if (NULL == service_bucket_s.array[i])
        {
            service_bucket_s.array[i] = p_data;
            service_bucket_s.cnt++;
            break;
        }
    }

    return 0;
}











// probably this will be external (triggered with GW_CONN_CB)
// might be put to app scheduler
void create_self_services_init(void)
{
    //generate self base64
    comm_utils_id_gen();


    //start the chain of creating->registering->subscribing all self services
    //iterate with endpoints and service types
    m_iter_endpoints = button_0;
    m_iter_services = info;

    // hit first self service creation



}

int8_t service_register(endpoint_t endpoint, service_type_t type)
{
    //pick self base64
    unsigned char * base_id = comm_utils_get_id();

    create_service_t * p_srv = service_create(base_id, endpoint, type);

    if (NULL == p_srv)
        return -1;

    // comm_manager_topic_register(ptr_topic, ptr_msg_id)


    return 0;
}




// change ID to uint8_t
create_service_t * service_create(unsigned char * p_base_id,
                                  endpoint_t endpoint,
                                  service_type_t type)
{
    if (NULL == p_base_id)
        return NULL;

    if (strlen(p_base_id) != BASE64_LENGTH)
        return NULL;

    if (endpoint >= endpoint_none)
        return NULL;

    if (type >= type_none)
        return NULL;

    // init/alloc the var
    create_service_t * p_data
        = (create_service_t *) malloc(size_of(create_service_t));

    if (NULL == p_data)
        return NULL;

    p_data->service.endpoint = endpoint;
    p_data->service.type = type;
    p_data->message_id = m_msg_id++;

    //serial var
    int8_t ret = mash_topic_name_serial(p_base_id, p_data);

    if (0 == ret)
    {
        return p_data;
    }
    else
    {
        free(p_data);
        return NULL;
    }
}
