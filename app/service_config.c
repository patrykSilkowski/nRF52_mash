/*
 * service_config.c
 *
 *  Created on: Jan 29, 2019
 *      Author: MSc Patryk Silkowski
 */


#include "service_config.h"

/* GCC */
#include <stdbool.h>
#include <string.h>
#include <stdio.h>


#define EXT_ENDPOINT_LENGTH             14
#define EXT_TOPIC_LIMIT                 20

// e.i. how many subs can get each endpoint
#define EXT_SUB_LIMIT_PER_ENDPOINT      8


// External subscribed topics type - so called 'group' container
typedef struct {
    char ext_endpoint_name[EXT_ENDPOINT_LENGTH + 1];  // +1 for '/0'
    uint16_t topic_id;
    endpoint_t endpoints[SERVICE_BSP_ENDPOINTS];
    uint8_t endpoints_cnt;  // assumed as a counter and an index of the next one
} ext_sub_topic_t;

// The subscription list type for each endpoint
typedef struct {
    char ext_endpoint_list [EXT_SUB_LIMIT_PER_ENDPOINT][EXT_ENDPOINT_LENGTH + 1];
    uint8_t sub_counter;
} ext_sub_list_t;


/* Structure containing temporary subscription info
 * Will receive the original topic ID within the SUBACK event
 *
 * msg_id acts as a link between event <=> service_setup <=> service-config
 */
static struct {
    char ext_endpoint_name[EXT_ENDPOINT_LENGTH + 1];  // +1 for '/0'
    uint16_t msg_id;
    endpoint_t self_endpoint;
} m_ext_sub_temp_s;

static ext_sub_topic_t m_ext_topics[EXT_TOPIC_LIMIT];
static uint8_t  m_ext_topics_cnt;

static ext_sub_list_t m_sub_list_arr[SERVICE_BSP_ENDPOINTS];

static bool m_is_initialized = false;


int8_t add_sub_to_endpoints_sub_list(endpoint_t endp,
                                     uint8_t * p_msg,
                                     uint16_t msg_length)
{
    if (EXT_ENDPOINT_LENGTH != msg_length)
        return -10;

    uint8_t cnt = m_sub_list_arr[endp].sub_counter;

    if (cnt >= EXT_SUB_LIMIT_PER_ENDPOINT)
        return -11;

    // check if already exists
    for (uint8_t i = 0; i < cnt; i++)
    {
        if (0 == strcmp((char*) p_msg,
                        m_sub_list_arr[endp].ext_endpoint_list[i]))
        {
            return -12;
        }
    }

    // write the ext endpoint name to the list
    int8_t err_code;
    err_code = (int8_t) snprintf(m_sub_list_arr[endp].ext_endpoint_list[cnt],
                                 msg_length + 1,
                                 "%s",
                                 p_msg);

    if (err_code < 0)
        return -13;

    m_sub_list_arr[endp].sub_counter++;
    return 0;
}


int8_t add_subscribed_ext_topic(uint16_t topic_id)
{
    uint8_t temp_cnt = m_ext_topics[m_ext_topics_cnt].endpoints_cnt;
    if (temp_cnt)      //endpoint counter of fresh topic should be zero!
        return -1;

    m_ext_topics[m_ext_topics_cnt].topic_id = topic_id;

    //add first endpoint of a new topic
    m_ext_topics[m_ext_topics_cnt].endpoints[temp_cnt]
                                             = m_ext_sub_temp_s.self_endpoint;

    int8_t err_code = snprintf(m_ext_topics[m_ext_topics_cnt].ext_endpoint_name,
                               EXT_ENDPOINT_LENGTH + 1,
                               "%s",
                               m_ext_sub_temp_s.ext_endpoint_name);

    if (err_code < 0)
    {
        return -2;
    }
    else
    {
        m_ext_topics[m_ext_topics_cnt].endpoints_cnt++;
        m_ext_topics_cnt++;
        return 0;
    }
}


bool is_ext_endpoint_name_valid(char * name, uint16_t * length)
{
    /*
     * Valid pattern --> example: s4t0dOpl8i2f/0
     *                   12 chars of base64, a slash and a number (0-9)
     */

    if (EXT_ENDPOINT_LENGTH != *length)
        return false;

    if ('/' != name[12])
        return false;

    if (   name[13] - '0' < 10
        && name[13] - '0' >= 0)
        return true;

    return false;
}


ext_sub_topic_t * is_ext_topic_subscribed(char * topic_name)
{
    if (0 == m_ext_topics_cnt)
        return NULL;

    for (uint8_t i = 0; i < m_ext_topics_cnt; i++)
    {
        if (0 == strcmp(topic_name, m_ext_topics[i].ext_endpoint_name))
            return &m_ext_topics[i];
    }

    return NULL;
}


int8_t add_endpoint_to_subscribed_ext_topic(endpoint_t endpoint,
                                            ext_sub_topic_t * p_ext_topic)
{
    // check if the endpoint is already set to the ext topic
    for (uint8_t i = 0; i < p_ext_topic->endpoints_cnt; i++)
    {
        if (p_ext_topic->endpoints[i] == endpoint)
            return -1;
    }

    p_ext_topic->endpoints[p_ext_topic->endpoints_cnt] = endpoint;
    p_ext_topic->endpoints_cnt++;

    return 0;
}


/*
 * Set the initial values on the external topics array
 */
void service_config_init(void)
{
    for (int i = 0; i < EXT_TOPIC_LIMIT; i++)
    {
        memset(m_ext_topics[i].endpoints,
               SERVICE_BSP_ENDPOINTS,
               sizeof(endpoint_t) * SERVICE_BSP_ENDPOINTS);
    }

    m_is_initialized = true;
}


/*
 * Ext stuff down there!
 */


int8_t service_config_subscribe(endpoint_t endpoint,
                                uint8_t * p_msg,
                                uint16_t msg_length)
{
    if (false == m_is_initialized)
        return -1;

    // check if the name is valid
    if (false == is_ext_endpoint_name_valid((char*) p_msg, &msg_length))
        return -2;

    ext_sub_topic_t * p_ext_sub = is_ext_topic_subscribed((char*) p_msg);

    // TODO pack n wrap
    int8_t err_code;
    if (NULL != p_ext_sub)
    {
        // add self endpoint to existing ext_topic (extend the group)
        err_code = add_endpoint_to_subscribed_ext_topic(endpoint, p_ext_sub);

        // the self endpoint is already engaged with ext topic
        if (err_code)
            return -3;

        // add ext endpoint to subscription list according to config_list
        err_code = add_sub_to_endpoints_sub_list(endpoint, p_msg, msg_length);

        return err_code;
    }

    // build a full topic name and subscribe to that one
    char ext_base64[BASE64_LENGTH + 1];
    int8_t ext_endpoint = p_msg[13] - '0';

    // the one and only service type which is handled by this device
    service_type_t type = onoff;

    err_code = (int8_t) snprintf(ext_base64,
                                 BASE64_LENGTH + 1,
                                 "%s",
                                 (char*) p_msg);

    if (err_code < 0)
        return -4;

    err_code = service_create(ext_base64, ext_endpoint, type);

    if (err_code)
        return -5;

    // save temp data before topic subscription
    if (service_is_created(&m_ext_sub_temp_s.msg_id))
    {
        m_ext_sub_temp_s.self_endpoint = endpoint;

        err_code = (int8_t) snprintf(m_ext_sub_temp_s.ext_endpoint_name,
                                     EXT_ENDPOINT_LENGTH + 1,
                                     "%s",
                                     (char*) p_msg);

        if (err_code < 0)
            return -4;
    }
    else
    {
        return -6;
    }

    err_code = service_subscribe();

    if (err_code)
        return -7;
    else
        return 0;
}


int8_t service_config_add_ext_topic(uint16_t ret_msg_id, uint16_t topic_id)
{
    uint16_t msg_id;
    if (!service_is_created(&msg_id))
        return -1;

    int8_t err_code;

    // ITS A MATCH!
    if (   ret_msg_id == msg_id
        && ret_msg_id == m_ext_sub_temp_s.msg_id)
    {
        /*
         * add to the database of external topics
         * (pass the topic ID and first self endpoint)
         */
        err_code = add_subscribed_ext_topic(topic_id);

        if (err_code)
            return -2;

        err_code = add_sub_to_endpoints_sub_list(m_ext_sub_temp_s.self_endpoint,
                         (uint8_t *) m_ext_sub_temp_s.ext_endpoint_name,
                         strlen((char*) m_ext_sub_temp_s.ext_endpoint_name));

        return err_code;
    }
    else
    {
        return -4;
    }
}
