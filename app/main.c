/*
    MQTT-SN pub/sub example on nRF52840 Thread - adapted from Nordic SDK
    examples.

    Buttons/LEDS as input/output devices (buttons are treated as switches)
    protocol usage presented in ../tools/protocol.odt
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_scheduler.h"
#include "app_timer.h"
#include "bsp_thread.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "mqttsn_client.h"
#include "thread_utils.h"

#include <openthread/thread.h>
#include <openthread/joiner.h>

#define VENDOR_NAME   "SIGMA_PS"
#define VENDOR_MODEL  "1"
#define VENDOR_SW_VER "0.0.1"
#define VENDOR_DATA   NULL
#define JOINER_PSKD   "J01NME"

#define DEFAULT_CHILD_TIMEOUT  40                                           /**< Thread child timeout [s]. */
#define DEFAULT_POLL_PERIOD    1000                                         /**< Thread Sleepy End Device polling period when MQTT-SN Asleep. [ms] */
#define NUM_SLAAC_ADDRESSES    4                                            /**< Number of SLAAC addresses. */
#define SEARCH_GATEWAY_TIMEOUT 100                                          /**< MQTT-SN Gateway discovery procedure timeout in [s]. */

#define SCHED_QUEUE_SIZE       32                                           /**< Maximum number of events in the scheduler queue. */
#define SCHED_EVENT_DATA_SIZE  APP_TIMER_SCHED_EVENT_DATA_SIZE              /**< Maximum app_scheduler event size. */

#define APP_TIM_JOINER_DELAY 200
#define APP_TIMER_TICKS_TIMEOUT APP_TIMER_TICKS(50)
//APP_TIMER_DEF(m_joiner_timer);

static mqttsn_client_t      m_client;                                       /**< An MQTT-SN client instance. */
static mqttsn_remote_t      m_gateway_addr;                                 /**< A gateway address. */
static uint8_t              m_gateway_id;                                   /**< A gateway ID. */
static mqttsn_connect_opt_t m_connect_opt;                                  /**< Connect options for the MQTT-SN client. */
static uint16_t             m_msg_id               = 0;                     /**< Message ID thrown with MQTTSN_EVENT_TIMEOUT. */

// THESE SHOULD BE CHANGED ACCORDING TO PROTOCOL ASSUMPTIONS
// CLIENT ID IS BASED ON EUI64
static otNetifAddress m_slaac_addresses[NUM_SLAAC_ADDRESSES];               /**< Buffer containing addresses resolved by SLAAC */

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

static bool g_sub_registered = false;
static bool g_led_2_on = false;
static bool g_led_3_on = false;


/***************************************************************************************************
 * @section scheduler prototypes
 **************************************************************************************************/

static void sched_send_report(void * p_event_data, uint16_t event_size);
static void sched_joiner(void * p_event_data, uint16_t event_size);
static void sched_get_ip(void * p_event_data, uint16_t event_size);
//static void sched_get_device_data(void * p_event_data, uint16_t event_size);


/***************************************************************************************************
 * @section app prototypes
 **************************************************************************************************/
static void joiner_start(void * p_context);
static void print_ip_info(void);

/***************************************************************************************************
 * @section MQTT-SN
 **************************************************************************************************/

/**@brief Turns the MQTT-SN network indication LED on.
 *
 * @details This LED is on when an MQTT-SN client is in connected or awake state.
 */
static void light_on(void)
{
    LEDS_ON(BSP_LED_3_MASK);
}


/**@brief Turns the MQTT-SN network indication LED off.
 *
 * @details This LED is on when an MQTT-SN client is in disconnected or asleep state.
 */
static void light_off(void)
{
    LEDS_OFF(BSP_LED_3_MASK);
}


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


/**@brief Processes GWINFO message from a gateway.
 *
 * @details This function updates MQTT-SN Gateway information.
 *
 * @param[in]    p_event  Pointer to MQTT-SN event.
 */
static void gateway_info_callback(mqttsn_event_t * p_event)
{
    m_gateway_addr = *(p_event->event_data.connected.p_gateway_addr);
    m_gateway_id   = p_event->event_data.connected.gateway_id;
}


/**@brief Processes CONNACK message from a gateway.
 *
 * @details This function launches the topic registration procedure if necessary.
 */
static void connected_callback(void)
{
    light_on();

    uint32_t err_code = mqttsn_client_topic_register(&m_client,
                                                     m_topic_pub.p_topic_name,
                                                     strlen(m_topic_name_pub),
                                                     &m_msg_id);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("REGISTER message could not be sent. Error code: 0x%x\r\n", err_code);
    }
}


/**@brief Processes DISCONNECT message from a gateway. */
static void disconnected_callback(void)
{
    light_off();
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
/**@brief Processes REGACK message from a gateway.
 *
 * @param[in] p_event Pointer to MQTT-SN event.
 */
static void regack_callback(mqttsn_event_t * p_event)
{
    NRF_LOG_INFO("MQTT-SN event: Topic has been registered with ID: %d.\r\n",
                 p_event->event_data.registered.packet.topic.topic_id);

    // register subscriber if not already registered
    if (!g_sub_registered)
    {
        m_topic_pub.topic_id = p_event->event_data.registered.packet.topic.topic_id;

        g_sub_registered = true;

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
}


/**@brief Processes retransmission limit reached event. */
static void timeout_callback(mqttsn_event_t * p_event)
{
    NRF_LOG_INFO("MQTT-SN event: Timed-out message: %d. Message ID: %d.\r\n",
                  p_event->event_data.error.msg_type,
                  p_event->event_data.error.msg_id);
}


/**@brief Processes results of gateway discovery procedure. */
static void searchgw_timeout_callback(mqttsn_event_t * p_event)
{
    NRF_LOG_INFO("MQTT-SN event: Gateway discovery result: 0x%x.\r\n", p_event->event_data.discovery);

    // if gateway was discovered, turn LED 2 on
    if (p_event->event_data.discovery == 0)
    {
        LEDS_ON(BSP_LED_2_MASK);
        g_led_2_on = true;
    }
}

/**@brief Processes data published by a broker.
 *
 * @details This function processes LED command.
 */
static void received_callback(mqttsn_event_t * p_event)
{
    if (p_event->event_data.published.packet.topic.topic_id == m_topic_sub.topic_id)
    {
        uint8_t* p_data = p_event->event_data.published.p_payload;
        NRF_LOG_INFO("MQTT-SN event: Content to subscribed topic received.\r\n");
        NRF_LOG_INFO("Topic id: %d, data: %5s",
            p_event->event_data.published.packet.topic.topic_id,
            p_data);

        // turn LEDs on/off
        if (p_data[0] == '1') {
            LEDS_ON(BSP_LED_2_MASK);
            g_led_2_on = true;
        }
        else {
            LEDS_OFF(BSP_LED_2_MASK);
            g_led_2_on = false;
        }
    }
    else
    {
        NRF_LOG_INFO("MQTT-SN event: Content to unsubscribed topic received. Dropping packet.\r\n");
    }
}

/**@brief Function for handling MQTT-SN events. */
void mqttsn_evt_handler(mqttsn_client_t * p_client, mqttsn_event_t * p_event)
{
    switch(p_event->event_id)
    {
        case MQTTSN_EVENT_GATEWAY_FOUND:
            NRF_LOG_INFO("MQTT-SN event: Client has found an active gateway.\r\n");
            gateway_info_callback(p_event);
            break;

        case MQTTSN_EVENT_CONNECTED:
            NRF_LOG_INFO("MQTT-SN event: Client connected.\r\n");
            connected_callback();
            break;

        case MQTTSN_EVENT_DISCONNECT_PERMIT:
            NRF_LOG_INFO("MQTT-SN event: Client disconnected.\r\n");
            disconnected_callback();
            break;

        case MQTTSN_EVENT_REGISTERED:
            NRF_LOG_INFO("MQTT-SN event: Client registered topic.\r\n");
            regack_callback(p_event);
            break;

        case MQTTSN_EVENT_PUBLISHED:
            NRF_LOG_INFO("MQTT-SN event: Client has successfully published content.\r\n");
            break;

        case MQTTSN_EVENT_SUBSCRIBED:
            NRF_LOG_INFO("MQTT-SN event: Client subscribed to topic.\r\n");
            break;

        case MQTTSN_EVENT_UNSUBSCRIBED:
            NRF_LOG_INFO("MQTT-SN event: Client unsubscribed to topic.\r\n");
            break;

        case MQTTSN_EVENT_RECEIVED:
            NRF_LOG_INFO("MQTT-SN event: Client received content.\r\n");
            received_callback(p_event);
            break;

        case MQTTSN_EVENT_TIMEOUT:
            NRF_LOG_INFO("MQTT-SN event: Retransmission retries limit has been reached.\r\n");
            timeout_callback(p_event);
            break;

        case MQTTSN_EVENT_SEARCHGW_TIMEOUT:
            NRF_LOG_INFO("MQTT-SN event: Gateway discovery procedure has finished.\r\n");
            searchgw_timeout_callback(p_event);
            break;

        default:
            break;
    }
}

/***************************************************************************************************
 * @section State
 **************************************************************************************************/

static void state_changed_callback(uint32_t flags, void * p_context)
{
    NRF_LOG_INFO("State changed! Flags: 0x%08x Current role: %d\r\n",
                 flags, otThreadGetDeviceRole(p_context));

    if (flags & OT_CHANGED_THREAD_NETDATA)
    {
        /**
         * Whenever Thread Network Data is changed, it is necessary to check if generation of global
         * addresses is needed. This operation is performed internally by the OpenThread CLI module.
         * To lower power consumption, the examples that implement Thread Sleepy End Device role
         * don't use the OpenThread CLI module. Therefore otIp6SlaacUpdate util is used to create
         * IPv6 addresses.
         */
         otIp6SlaacUpdate(thread_ot_instance_get(), m_slaac_addresses,
                          sizeof(m_slaac_addresses) / sizeof(m_slaac_addresses[0]),
                          otIp6CreateRandomIid, NULL);
    }

    app_sched_event_put(NULL, 0, sched_send_report);
}

static void publish(void)
{
    char* pub_data = g_led_2_on ? "1" : "0";
    uint32_t err_code = mqttsn_client_publish(&m_client, m_topic_pub.topic_id,
        (uint8_t*)pub_data, strlen(pub_data), &m_msg_id);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("PUBLISH message could not be sent. Error code: 0x%x\r\n", err_code)
    }
    else
    {
        NRF_LOG_INFO("PUBLISH successfully sent.");
    }
}

static void bsp_event_handler(bsp_event_t event)
{
    if (otThreadGetDeviceRole(thread_ot_instance_get()) < OT_DEVICE_ROLE_CHILD)
    {
        (void)event;
        return;
    }
    
    switch (event)
    {
        case BSP_EVENT_KEY_1:
        {
            // reset LED state
            LEDS_OFF(BSP_LED_2_MASK);
            g_led_2_on = false;

            uint32_t err_code = mqttsn_client_search_gateway(&m_client, SEARCH_GATEWAY_TIMEOUT);
            if (err_code != NRF_SUCCESS)
            {
                NRF_LOG_ERROR("SEARCH GATEWAY message could not be sent. Error: 0x%x\r\n", err_code);
            }
            else
            {
                NRF_LOG_INFO("SEARCH GATEWAY message sent.");
            }
        }
        break;

        case BSP_EVENT_KEY_2:
        {
            uint32_t err_code;

            if (mqttsn_client_state_get(&m_client) == MQTTSN_CLIENT_CONNECTED)
            {
                err_code = mqttsn_client_disconnect(&m_client);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_ERROR("DISCONNECT message could not be sent. Error: 0x%x\r\n", err_code);
                }
                else
                {
                    //LEDS_OFF(BSP_LED_3_MASK);
                    g_led_3_on = false;
                    NRF_LOG_INFO("DISCONNECT MQTT-SN CLIENT message sent.");
                }
            }
            else
            {
                err_code = mqttsn_client_connect(&m_client, &m_gateway_addr, m_gateway_id, &m_connect_opt);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_ERROR("CONNECT message could not be sent. Error: 0x%x\r\n", err_code);
                }
                else
                {
                    //LEDS_ON(BSP_LED_3_MASK);
                    g_led_3_on = true;
                    NRF_LOG_INFO("CONNECT MQTT-SN CLIENT message sent.");
                }
            }
        }
        break;

        case BSP_EVENT_KEY_3:
        {
            publish();
        }
        break;

        default:
        break;
    } // end of switch
}

/***************************************************************************************************
 * @section Initialization
 **************************************************************************************************/

/**@brief Function for initializing the Application Timer Module.
 */
static void timer_init(void)
{
    uint32_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the LEDs.
 */
static void leds_init(void)
{
    LEDS_CONFIGURE(LEDS_MASK);
    LEDS_OFF(LEDS_MASK);
}


/**@brief Function for initializing the buttons and their event handler.
 */
static void buttons_bsp_init(void)
{
    uint32_t err_code;
    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing the Thread Stack.
 */
static void thread_instance_init(void)
{
    thread_configuration_t thread_configuration =
    {
        .role                  = RX_ON_WHEN_IDLE,
        .autocommissioning     = false,
        .poll_period           = DEFAULT_POLL_PERIOD,
        .default_child_timeout = DEFAULT_CHILD_TIMEOUT,
    };

    thread_init(&thread_configuration);
    //thread_cli_init();  // DONT NEED THIS ANYMORE ->
    thread_state_changed_callback_set(state_changed_callback);
}


/**@brief Function for initializing the MQTTSN client.
 */
static void mqttsn_init(void)
{
    uint32_t err_code = mqttsn_client_init(&m_client,
                                           MQTTSN_DEFAULT_CLIENT_PORT,
                                           mqttsn_evt_handler,
                                           thread_ot_instance_get());
    APP_ERROR_CHECK(err_code);

    connect_opt_init();
}

static void report_on_network()
{
    return; // MB wrote UDP handling here
}


/**@brief Function for initializing scheduler module.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

static void sched_send_report(void * p_event_data, uint16_t event_size)
{
    report_on_network();
}
static void sched_joiner(void * p_event_data, uint16_t event_size)
{
    joiner_start(NULL);
}
static void sched_get_ip(void * p_event_data, uint16_t event_size)
{
    print_ip_info();
}

/*
static void sched_get_device_data(void * p_event_data, uint16_t event_size)
{
    return;
}
*/

/**@brief Function for triggering timer which handles the joiner state.
 */
 /*
static uint32_t start_joiner_timer(void)
{
    return app_timer_start(m_joiner_timer,
                           APP_TIMER_TICKS(APP_TIM_JOINER_DELAY),
                           thread_ot_instance_get());
}
*/

static int join_tries = 20;
/**@brief Callback function for initializing the joiner state.
 */
static void joiner_callback(otError aError, void *aContext)
{
    switch (aError)
    {
        case OT_ERROR_NONE:
            NRF_LOG_INFO("Joiner: success");
            aError = otThreadSetEnabled(thread_ot_instance_get(), true);
            //print_ip_info();
            ASSERT(aError == OT_ERROR_NONE);
        break;

        case OT_ERROR_SECURITY:
            NRF_LOG_ERROR("Joiner: failed - credentials");
        break;

        case OT_ERROR_NOT_FOUND:
            NRF_LOG_ERROR("Joiner: failed - no network");
        break;

        case OT_ERROR_RESPONSE_TIMEOUT:
            NRF_LOG_ERROR("Joiner: failed - timeout");
        break;

        default:
        break;
    }

    if (aError != OT_ERROR_NONE)
    {
        join_tries--;

        if (join_tries > 0)
        {
            uint32_t err_code = app_sched_event_put(NULL, 0, sched_joiner);
            APP_ERROR_CHECK(err_code);
        }
        else
        {
            NRF_LOG_ERROR("Commissioning failed!");
        }
    }
}


/**@brief Function for initializing the joiner state.
 */
static void joiner_start(void * p_context)
{
    otError ret = otJoinerStart(thread_ot_instance_get(),
                                JOINER_PSKD,
                                NULL,
                                VENDOR_NAME,
                                VENDOR_MODEL,
                                VENDOR_SW_VER,
                                VENDOR_DATA,
                                joiner_callback,
                                p_context);

    switch(ret)
    {
        case OT_ERROR_NONE:
            NRF_LOG_INFO("Joiner: start success");
        break;

        case OT_ERROR_INVALID_ARGS:
            NRF_LOG_ERROR("Joiner: aPSKd or a ProvisioningUrl is invalid.");
        break;

        case OT_ERROR_DISABLED_FEATURE:
            NRF_LOG_ERROR("Joiner: is not enabled");
        break;

        default:
        break;
    }
}


/**@brief Function for formating IPv6 from typedef to string.
 */
void format_ip6(char *str, const otIp6Address* addr)
{
    if(addr)
    {
        sprintf(str,"%x:%x:%x:%x:%x:%x:%x:%x",
                addr->mFields.m16[0],
                addr->mFields.m16[1],
                addr->mFields.m16[2],
                addr->mFields.m16[3],
                addr->mFields.m16[4],
                addr->mFields.m16[5],
                addr->mFields.m16[6],
                addr->mFields.m16[7]);
    }
    else
    {
        sprintf(str,"NULL");
    }
}


/**@brief Function for printing IP information via NRF_LOG(RTT).
 */
static void print_ip_info(void)
{
    char buff[128];
    const otNetifMulticastAddress *mulAddress
        = otIp6GetMulticastAddresses(thread_ot_instance_get());

    //otNetifMulticastAddress **_mulAddr = &mulAddress;
    //otNetifAddress uniAddress = otIp6GetUnicastAdresses(thread_ot_instance_get());
    if(mulAddress)
    {
        for(; mulAddress/*->mNext*/; mulAddress = mulAddress->mNext)
        {
            format_ip6(buff,&(mulAddress->mAddress));
            NRF_LOG_INFO("Multicast Address: %s", nrf_log_push(buff));
            NRF_LOG_PROCESS();
        }
    }

    const otNetifAddress *uniAddress
        = otIp6GetUnicastAddresses(thread_ot_instance_get());

    if(uniAddress)
    {
        for(; uniAddress/*->mNext*/; uniAddress=uniAddress->mNext)
        {
            format_ip6(buff,&(uniAddress->mAddress));
            NRF_LOG_INFO("Unicast Address: %s", nrf_log_push(buff));
            NRF_LOG_PROCESS();
        }
    }
}


/**@brief Function for checking the commissioning status.
 */
static void commissioning_check(void)
{
    //return; // TODO FIX ME

    if (!otDatasetIsCommissioned(thread_ot_instance_get()))
    {
        NRF_LOG_ERROR("Device is not commissioned yet!");
        app_sched_event_put(NULL, 0, sched_joiner);
    }
    else
    {
        NRF_LOG_INFO("Device successfully commissioned!");
        app_sched_event_put(NULL, 0, sched_get_ip);
    }

    NRF_LOG_PROCESS();
}

static char str_eui[17];

/**@brief Function for initializing device information
 * (eui64 and QRcode to generate pattern)
 */
static void device_info_init(void)
{
    otExtAddress eui64;

    otLinkGetFactoryAssignedIeeeEui64(thread_ot_instance_get(), &eui64);

    NRF_LOG_PROCESS();
    NRF_LOG_FLUSH();

    sprintf(str_eui,"%x%x%x%x%x%x%x%x",
            eui64.m8[0],
            eui64.m8[1],
            eui64.m8[2],
            eui64.m8[3],
            eui64.m8[4],
            eui64.m8[5],
            eui64.m8[6],
            eui64.m8[7]);

    NRF_LOG_INFO("EUI64:%s",str_eui);
    NRF_LOG_INFO("QRCODE: v=1&&eui=%s&&cc=%s", str_eui, JOINER_PSKD);
    NRF_LOG_INFO("sudo wpanctl commissioner -a %s %s", JOINER_PSKD, str_eui);
    NRF_LOG_PROCESS();
}

/***************************************************************************************************
 * @section Main
 **************************************************************************************************/

int main(int argc, char *argv[])
{
    log_init();
    scheduler_init();
    timer_init();
    leds_init();
    buttons_bsp_init();

/*
 *  mash_id_gen();
    NRF_LOG_INFO("ID:%s",mash_get_id());
 */

    thread_instance_init();
    //init_joiner_timer();
    commissioning_check();
    device_info_init();
    mqttsn_init();
    //start_joiner_timer();

    while (true)
    {
        thread_process();
        app_sched_execute();

        if (NRF_LOG_PROCESS() == false)
        {
            thread_sleep();
        }
    }
}

/**
 *@}
 **/
