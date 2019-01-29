/*
 * service_setup.c
 *
 *  Created on: Jan 22, 2019
 *      Author: MSc Patryk Silkowski
 */


#include "service_setup.h"

/* GCC */
#include <string.h>
#include <stdio.h>

/* APP */
#include "comm_manager.h"
#include "comm_utils.h"


#define SERVICE_DATA_ARRAY_SIZE       60
#define SERVICE_CREATE_BUFFER_SIZE    4
#define BASE64_LENGTH                 12

#define MQTTSN_TOPIC_NAME_LENGTH      32
#define DEFAULT_RETRANSMISSION_CNT    4


#define SERVICE_STR_INFO          "info"
#define SERVICE_STR_TIME          "time"
#define SERVICE_STR_PREC          "prec"
#define SERVICE_STR_ONOFF         "onoff"
#define SERVICE_STR_TEMPHUM       "temphum"
#define SERVICE_STR_CONFIG_SUB    "config/sub"
#define SERVICE_STR_CONFIG_UNSUB  "config/unsub"
#define SERVICE_STR_CONFIG_LIST   "config/list"


typedef struct {
    service_data_t  service;
    char            topic_name[MQTTSN_TOPIC_NAME_LENGTH];
    uint16_t        message_id;
    uint8_t         retry_cnt;
    bool            is_created;
} create_service_t;


static service_data_t   m_service_database[SERVICE_DATA_ARRAY_SIZE];
static uint8_t          m_service_cnt     =  0;


static endpoint_t       m_iter_endpoints  =  0;
static service_type_t   m_iter_services   =  0;

static uint16_t         m_msg_ids         =  0;
static create_service_t m_srv_setup       = {0};


void database_add(service_data_t * p_data)
{
    if (NULL == p_data)
        return;

    m_service_database[m_service_cnt] = *p_data;
    m_service_cnt++;
}


void database_delete_with_topic_id(uint16_t topic_id)
{
    //here will be set binary search probably
}

service_data_t * database_pop_with_topic_id(uint16_t topic_id)
{
    //here will be set binary search probably
    return NULL;
}



int8_t mash_topic_name_serial(char * id_str, create_service_t * dataset)
{
    char str_service_type[SERVICE_STR_MAX_LENGTH] = {0};
    int8_t ret;

    switch (dataset->service.type)
    {
        case info:
            ret = (int8_t) snprintf(str_service_type,
                                    SERVICE_STR_MAX_LENGTH,
                                    SERVICE_STR_INFO);
        break;

        case onoff:
            ret = (int8_t) snprintf(str_service_type,
                                    SERVICE_STR_MAX_LENGTH,
                                    SERVICE_STR_ONOFF);
        break;

        case config_sub:
            ret = (int8_t) snprintf(str_service_type,
                                    SERVICE_STR_MAX_LENGTH,
                                    SERVICE_STR_CONFIG_SUB);
        break;

        case config_unsub:
            ret = (int8_t) snprintf(str_service_type,
                                    SERVICE_STR_MAX_LENGTH,
                                    SERVICE_STR_CONFIG_UNSUB);
        break;

        case config_list:
            ret = (int8_t) snprintf(str_service_type,
                                    SERVICE_STR_MAX_LENGTH,
                                    SERVICE_STR_CONFIG_LIST);
        break;

        case type_none:
        default:
            return -1;
        break;
    } // end of switch

    if (ret < 0)
        return -1;

    ret = (int8_t) snprintf(dataset->topic_name,
                            MQTTSN_TOPIC_NAME_LENGTH,
                            "%s//%d//%s",
                            id_str,
                            (uint8_t) dataset->service.endpoint,
                            str_service_type);

    if (ret < 0)
        return -1;
    else
        return 0;
}


// change ID to uint8_t
int8_t service_create(char * p_base_id,
                      endpoint_t endpoint,
                      service_type_t type)
{
    if (NULL == p_base_id)
        return -1;

    if (strlen(p_base_id) != BASE64_LENGTH)
        return -2;

    if (endpoint >= endpoint_none)
        return -3;

    if (type >= type_none)
        return -4;

    memset(&m_srv_setup, 0, sizeof(create_service_t));

    m_srv_setup.service.endpoint = endpoint;
    m_srv_setup.service.type = type;
    m_srv_setup.message_id = m_msg_ids++;
    m_srv_setup.is_created = true;

    //serial var
    return mash_topic_name_serial(p_base_id, &m_srv_setup);
}


//static inline (?)
void service_destroy(create_service_t * p_data)
{
    memset(&m_srv_setup, 0, sizeof(create_service_t));
}


int8_t service_register(void)
{
    if (false == m_srv_setup.is_created)
        return -1;

    return comm_manager_topic_register(m_srv_setup.topic_name,
                                       &m_srv_setup.message_id);
}

int8_t service_subscribe(void)
{
    if (false == m_srv_setup.is_created)
        return -1;

    return comm_manager_topic_subscribe(m_srv_setup.topic_name,
                                        &m_srv_setup.message_id);
}

int8_t service_subscribe_to_registered(uint16_t msg_id, uint16_t topic_id)
{
    //check message ID
    if (m_srv_setup.message_id != msg_id)
        return -2;

    //change message ID
    m_srv_setup.message_id = m_msg_ids++;

    //store the topic ID (probably do not need this atm)
    m_srv_setup.service.topic_id = topic_id;

    return service_subscribe();
}

int8_t service_insert_to_database(uint16_t msg_id, uint16_t topic_id)
{
    if (false == m_srv_setup.is_created)
        return -1;

    //check message ID
    if (m_srv_setup.message_id != msg_id)
        return -2;

    //check if topic is already there
    if (0 == m_srv_setup.service.topic_id)
        m_srv_setup.service.topic_id = topic_id;  //store the topic ID

    //check if topic ID matches with the one previously stored
    if (m_srv_setup.service.topic_id != topic_id)
        return -3;

    //add to database
    database_add(&m_srv_setup.service);
    return 0;
}

int8_t service_retry_register(uint16_t msg_id)
{
    //check message ID
    if (m_srv_setup.message_id != msg_id)
        return -2;

    m_srv_setup.retry_cnt++;
    if (DEFAULT_RETRANSMISSION_CNT == m_srv_setup.retry_cnt)
    {
        //shit happens... consider disconnecting from the gateway
        return SERVICE_RETRY_CNT_MAX_FLAG;
    }

    return service_register();
}

int8_t service_retry_subscribe(uint16_t msg_id)
{
    //check message ID
    if (m_srv_setup.message_id != msg_id)
        return -2;

    m_srv_setup.retry_cnt++;
    if (DEFAULT_RETRANSMISSION_CNT == m_srv_setup.retry_cnt)
    {
        //shit happens... consider disconnecting from the gateway
        return SERVICE_RETRY_CNT_MAX_FLAG;
    }

    return service_subscribe();
}

// probably this will be external (triggered with GW_CONN_CB)
// might be put to app scheduler
int8_t create_self_services_init(void)
{
    //generate self base64
    comm_utils_id_gen();


    //start the chain of creating->registering->subscribing all self services
    //iterate with endpoints and service types
    m_iter_endpoints = button_0;
    m_iter_services = info;

    // hit first self service creation
    int8_t ret = service_create(comm_utils_get_id(),
                                m_iter_endpoints,
                                m_iter_services);

    if (!ret)
    {
        ret = service_register();
    }

    return ret;
}

int8_t create_self_services_continue(void)
{
    if (   m_iter_endpoints  == endpoint_none
        && m_iter_services   == type_none    )
        return 0;   //all self services already registered!

    m_iter_services++;

    if (type_none == m_iter_services)
    {
        m_iter_endpoints++;

        if (endpoint_none == m_iter_endpoints)
        {
            return SERVICE_ALL_REGISTERED_FLAG;
        }
        else
        {
            m_iter_services = info;
        }
    }

    // hit next self service creation
    int8_t ret = service_create(comm_utils_get_id(),
                                m_iter_endpoints,
                                m_iter_services);

    if (!ret)
    {
        ret = service_register();
    }

    return ret;
}
