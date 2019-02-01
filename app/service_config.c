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

#define EXT_TOPIC_ENDPOINT_LENGTH   14
#define EXT_TOPIC_LIMIT             20


// External subscribed topics type
typedef struct {
    char ext_endpoint_name[EXT_TOPIC_ENDPOINT_LENGTH + 1];  // +1 for '/0'
    uint16_t topic_id;
    endpoint_t endpoints[SERVICE_BSP_ENDPOINTS];
    uint8_t endpoints_cnt;  // assumed as a counter and an index of the next one
} ext_sub_topics_t;


static ext_sub_topics_t m_ext_topics[EXT_TOPIC_LIMIT];
static uint8_t  m_ext_topics_cnt;
static bool m_is_initialized = false;


bool is_ext_endpoint_name_valid(char * name, uint16_t * length)
{
    /*
     * Valid pattern --> example: s4t0dOpl8i2f/0
     *                   12 chars of base64, a slash and a number (0-9)
     */

    if (EXT_TOPIC_ENDPOINT_LENGTH != *length)
        return false;

    if ('/' != name[12])
        return false;

    if (-1 < name[13] - '0' < 10)
        return true;

    return false;
}



ext_sub_topics_t * is_ext_topic_subscribed(char * topic_name)
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
                                            ext_sub_topics_t * p_ext_topic)
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
               endpoint_none,
               sizeof(endpoint_t) * SERVICE_BSP_ENDPOINTS);
    }

    m_is_initialized = true;
}




int8_t service_config_subscribe(endpoint_t endpoint,
                                uint8_t * p_msg,
                                uint16_t msg_length)
{
    if (false == m_is_initialized)
        return -1;

    // check if the name is valid
    if (false == is_ext_endpoint_name_valid((char*) p_msg, &msg_length))
        return -2;

    ext_sub_topics_t * p_ext_sub = is_ext_topic_subscribed((char*) p_msg);

    int8_t err_code;
    if (NULL != p_ext_sub)
    {
        // add endpoint to existing ext_topic
        err_code = add_endpoint_to_subscribed_ext_topic(endpoint, p_ext_sub);

        if (err_code)
        {
            // the endpoint is already engaged with ext topic
            return -3;
        }
        else
        {
            // successfully added endpoint to already subscribed ext topic
            return 0;
        }
    }

    // build a full topic name and subscribe to that one


    return 0;
}
