/*
 * comm_manager.c
 *
 *  Created on: Jan 22, 2019
 *      Author: MSc Patryk Silkowski
 */

#include "comm_manager.h"

/* GCC */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* SDK */
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


#define SEARCH_GATEWAY_TIMEOUT      100                                     /**< MQTT-SN Gateway discovery procedure timeout in [s]. */
#define SEARCH_GATEWAY_TRIES        20                                      /**< Amount of attempts to connect to the MQTT-SN gateway */

#define MQTTSN_EVENT_COUNT          16                                      /**< Amount of MQTT-SN events. */

static mqttsn_client_t      m_client;                                       /**< An MQTT-SN client instance. */
static mqttsn_remote_t      m_gateway_addr;                                 /**< A gateway address. */
static uint8_t              m_gateway_id;                                   /**< A gateway ID. */
static mqttsn_connect_opt_t m_connect_opt;                                  /**< Connect options for the MQTT-SN client. */
static uint16_t             m_msg_id               = 0;                     /**< Message ID thrown with MQTTSN_EVENT_TIMEOUT. */

// THESE SHOULD BE CHANGED ACCORDING TO PROTOCOL ASSUMPTIONS
// CLIENT ID IS BASED ON EUI64

static char                 m_client_id[]          = "nRF52840";            /**< The MQTT-SN Client's ID. */
static char                 m_topic_name_pub[]     = "nRF52840/data";       /**< Name of the topic corresponding to subscriber's BSP_LED_2. */
static mqttsn_topic_t       m_topic_pub            =                        /**< Topic corresponding to subscriber's BSP_LED_2. */
{
    .p_topic_name = (unsigned char *)m_topic_name_pub,
    .topic_id     = 0,
};

static char                 m_topic_name_sub[]     = "nRF52840/cmd";        /**< Name of the topic corresponding to subscriber's BSP_LED_2. */
static mqttsn_topic_t       m_topic_sub            =                        /**< Topic corresponding to subscriber's BSP_LED_2. */
{
    .p_topic_name = (unsigned char *)m_topic_name_sub,
    .topic_id     = 0,
};

static comm_manager_event_cb m_event_cb[MQTTSN_EVENT_COUNT];

/***************************************************************************************************
 * @section MQTT-SN handling
 **************************************************************************************************/


/**@brief Initializes MQTT-SN client's connection options.
 */
static void connect_opt_init(void)
{
    m_connect_opt.alive_duration = MQTTSN_DEFAULT_ALIVE_DURATION,
    m_connect_opt.clean_session  = MQTTSN_DEFAULT_CLEAN_SESSION_FLAG,
    m_connect_opt.will_flag      = MQTTSN_DEFAULT_WILL_FLAG,
    m_connect_opt.client_id_len  = strlen(m_client_id),

    memcpy(m_connect_opt.p_client_id, (unsigned char *)m_client_id, m_connect_opt.client_id_len);
}


static void subscribe()
{
    uint8_t  topic_name_len = strlen(m_topic_name_sub);
    uint32_t err_code = mqttsn_client_subscribe(&m_client,
        m_topic_sub.p_topic_name, topic_name_len, &m_msg_id);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("SUBSCRIBE message could not be sent.\r\n");
    }
    else
    {
        NRF_LOG_INFO("SUBSCRIBE message successfully sent.");
    }
}



static void execute_callback(mqttsn_event_t * p_event)
{
    if (m_event_cb[p_event->event_id])
    {
        int8_t ret_cb = m_event_cb[p_event->event_id](p_event);

        if (ret_cb != CONN_MGR_SUCCESS)
        {
            NRF_LOG_ERROR("MQTT-SN event callback returned with error!");
        }
    }
}


/**@brief Processes GWINFO message from a gateway.
 *
 * @details This function updates MQTT-SN Gateway information.
 *
 * @param[in]    p_event  Pointer to MQTT-SN event.
 */
static void evt_gateway_found(mqttsn_event_t * p_event)
{
    m_gateway_addr = *(p_event->event_data.connected.p_gateway_addr);
    m_gateway_id   = p_event->event_data.connected.gateway_id;

    execute_callback(p_event);
}


/**@brief Processes CONNACK message from a gateway.
 *
 * @details This function launches the topic registration procedure if necessary.
 */
static void evt_connected(mqttsn_event_t * p_event)
{

    // TODO
    // THIS IS THE PLASE WHERE THE CREATION OF SELF SERVICES WILL BE
    // INITIALIZED
    uint32_t err_code = mqttsn_client_topic_register(&m_client,
                                                     m_topic_pub.p_topic_name,
                                                     strlen(m_topic_name_pub),
                                                     &m_msg_id);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("REGISTER message could not be sent. Error code: 0x%x\r\n", err_code);
    }

    execute_callback(p_event);
}


/**@brief Processes DISCONNECT message from a gateway. */
static void evt_disconnect_permit(mqttsn_event_t * p_event)
{
    execute_callback(p_event);
}


/**@brief Processes REGACK message from a gateway.
 *
 * @param[in] p_event Pointer to MQTT-SN event.
 */
static void evt_registered(mqttsn_event_t * p_event)
{
    NRF_LOG_INFO("MQTT-SN event: Topic has been registered with ID: %d.\r\n",
                 p_event->event_data.registered.packet.topic.topic_id);

    // register subscriber if not already registered
    if (true)
    {
        m_topic_pub.topic_id = p_event->event_data.registered.packet.topic.topic_id;

        uint32_t err_code = mqttsn_client_topic_register(&m_client,
                                                     m_topic_sub.p_topic_name,
                                                     strlen(m_topic_name_sub),
                                                     &m_msg_id);
        if (err_code != NRF_SUCCESS)
        {
            NRF_LOG_ERROR("REGISTER message could not be sent. Error code: 0x%x\r\n", err_code);
        }
        else
        {
            NRF_LOG_INFO("REGISTER message successfully sent.");
        }
    }
    else
    {
        // store id
        m_topic_sub.topic_id = p_event->event_data.registered.packet.topic.topic_id;

        // subscribe
        subscribe();
    }

    execute_callback(p_event);
}


/**@brief Processes PUBACK message from a gateway.
 *
 * @param[in] p_event Pointer to MQTT-SN event.
 */
static void evt_published(mqttsn_event_t * p_event)
{
    execute_callback(p_event);
}


/**@brief Processes SUBACK message from a gateway.
 *
 * @param[in] p_event Pointer to MQTT-SN event.
 */
static void evt_subscribed(mqttsn_event_t * p_event)
{
    execute_callback(p_event);
}

/**@brief Processes UNSUBACK message from a gateway.
 *
 * @param[in] p_event Pointer to MQTT-SN event.
 */
static void evt_unsubscribed(mqttsn_event_t * p_event)
{
    execute_callback(p_event);
}


/**@brief Processes PUBLISH message from a gateway.
 *
 * @param[in] p_event Pointer to MQTT-SN event
 */
static void evt_received(mqttsn_event_t * p_event)
{
    if (p_event->event_data.published.packet.topic.topic_id == m_topic_sub.topic_id)
    {
        uint8_t* p_data = p_event->event_data.published.p_payload;
        NRF_LOG_INFO("MQTT-SN event: Content to subscribed topic received.\r\n");
        NRF_LOG_INFO("Topic id: %d, data: %5s",
            p_event->event_data.published.packet.topic.topic_id,
            p_data);

    }
    else
    {
        NRF_LOG_INFO("MQTT-SN event: Content to unsubscribed topic received. Dropping packet.\r\n");
    }

    execute_callback(p_event);
}


/**@brief Processes retransmission limit reached event.
 * (MQTTSN_RC_REJECTED_CONGESTED)
 *
 * @param[in] p_event Pointer to MQTT-SN event
 */
static void evt_timeout(mqttsn_event_t * p_event)
{
    NRF_LOG_INFO("MQTT-SN event: Timed-out message: %d. Message ID: %d.\r\n",
                  p_event->event_data.error.msg_type,
                  p_event->event_data.error.msg_id);

    execute_callback(p_event);
}


/**@brief Processes the event of time expired to search the gateway.
 *
 * @details This function will try to search for the gateway once again.
 *
 * @details Bowels within mqttsn_gateway_discovery.c
 *
 * @param[in]    p_event  Pointer to MQTT-SN event.
 */
static void evt_search_gateway_timeout(mqttsn_event_t * p_event)
{
    NRF_LOG_INFO("MQTT-SN event: Gateway discovery result: 0x%x.\r\n",
                 p_event->event_data.discovery);

    execute_callback(p_event);
}


/**@brief Function for handling MQTT-SN events. */
static void mqttsn_evt_handler(mqttsn_client_t * p_client, mqttsn_event_t * p_event)
{
    switch(p_event->event_id)
    {
        case MQTTSN_EVENT_GATEWAY_FOUND:
            NRF_LOG_INFO("MQTT-SN event: Client has found an active gateway.\r\n");
            evt_gateway_found(p_event);
        break;

        case MQTTSN_EVENT_CONNECTED:
            NRF_LOG_INFO("MQTT-SN event: Client connected.\r\n");
            evt_connected(p_event);
        break;

        case MQTTSN_EVENT_DISCONNECT_PERMIT:
            NRF_LOG_INFO("MQTT-SN event: Client disconnected.\r\n");
            evt_disconnect_permit(p_event);
        break;

        case MQTTSN_EVENT_REGISTERED:
            NRF_LOG_INFO("MQTT-SN event: Client registered topic.\r\n");
            evt_registered(p_event);
        break;

        case MQTTSN_EVENT_PUBLISHED:
            NRF_LOG_INFO("MQTT-SN event: Client has successfully published content.\r\n");
            evt_published(p_event);
        break;

        case MQTTSN_EVENT_SUBSCRIBED:
            NRF_LOG_INFO("MQTT-SN event: Client subscribed to topic.\r\n");
            evt_subscribed(p_event);
        break;

        case MQTTSN_EVENT_UNSUBSCRIBED:
            NRF_LOG_INFO("MQTT-SN event: Client unsubscribed to topic.\r\n");
            evt_unsubscribed(p_event);
        break;

        case MQTTSN_EVENT_RECEIVED:
            NRF_LOG_INFO("MQTT-SN event: Client received content.\r\n");
            evt_received(p_event);
        break;

        case MQTTSN_EVENT_TIMEOUT:
            NRF_LOG_INFO("MQTT-SN event: Retransmission retries limit has been reached.\r\n");
            evt_timeout(p_event);
        break;

        case MQTTSN_EVENT_SEARCHGW_TIMEOUT:
            NRF_LOG_INFO("MQTT-SN event: Gateway discovery procedure has finished.\r\n");
            evt_search_gateway_timeout(p_event);
        break;

        default:
            NRF_LOG_ERROR("MQTT-SN event: Unsupported event occured.\r\n");
        break;
    }
}














/**@brief Function for initializing the MQTTSN client.
 */
void comm_manager_mqttsn_init(const void * p_transport)
{
    uint32_t err_code = mqttsn_client_init(&m_client,
                                           MQTTSN_DEFAULT_CLIENT_PORT,
                                           mqttsn_evt_handler,
                                           p_transport);
    APP_ERROR_CHECK(err_code);

    connect_opt_init();
}

/**@brief Function for searching the MQTTSN gateway.
 */
void comm_manager_search_gateway(void)
{
    uint32_t err_code = mqttsn_client_search_gateway(&m_client, SEARCH_GATEWAY_TIMEOUT);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("MQTT-SN message: search gateway could not be sent. Error: 0x%x\r\n", err_code);
    }
    else
    {
        NRF_LOG_INFO("MQTT-SN message: search gateway sent.");
    }
}


void comm_manager_set_evt_gateway_found_cb(comm_manager_event_cb cb)
{
    m_event_cb[MQTTSN_EVENT_GATEWAY_FOUND] = cb;
}


void comm_manager_set_evt_connected_cb(comm_manager_event_cb cb)
{
    m_event_cb[MQTTSN_EVENT_CONNECTED] = cb;
}


void comm_manager_set_evt_disconnect_permit_cb(comm_manager_event_cb cb)
{
    m_event_cb[MQTTSN_EVENT_DISCONNECT_PERMIT] = cb;
}

void comm_manager_set_evt_registered_cb(comm_manager_event_cb cb)
{
    m_event_cb[MQTTSN_EVENT_REGISTERED] = cb;
}

void comm_manager_set_evt_published_cb(comm_manager_event_cb cb)
{
    m_event_cb[MQTTSN_EVENT_PUBLISHED] = cb;
}


void comm_manager_set_evt_subscribed_cb(comm_manager_event_cb cb)
{
    m_event_cb[MQTTSN_EVENT_SUBSCRIBED] = cb;
}


void comm_manager_set_evt_unsubscribed_cb(comm_manager_event_cb cb)
{
    m_event_cb[MQTTSN_EVENT_UNSUBSCRIBED] = cb;
}


void comm_manager_set_evt_received_cb(comm_manager_event_cb cb)
{
    m_event_cb[MQTTSN_EVENT_RECEIVED] = cb;
}


void comm_manager_set_evt_timeout_cb(comm_manager_event_cb cb)
{
    m_event_cb[MQTTSN_EVENT_TIMEOUT] = cb;
}


void comm_manager_set_evt_gateway_search_timeout_cb(comm_manager_event_cb cb)
{
    m_event_cb[MQTTSN_EVENT_SEARCHGW_TIMEOUT] = cb;
}

