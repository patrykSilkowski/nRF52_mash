/*
    MQTT-SN pub/sub example on nRF52840 Thread - adapted from Nordic SDK
    examples.

    Buttons/LEDS as input/output devices (buttons are treated as switches)
    protocol usage presented in ../tools/protocol.odt
 */

/* GCC */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* SDK */
#include "app_scheduler.h"
#include "app_timer.h"
#include "bsp_thread.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "thread_utils.h"

#include <openthread/thread.h>
#include <openthread/joiner.h>

/* APP */
#include "comm_manager.h"
#include "comm_utils.h"

#define VENDOR_NAME   "SIGMA_PS"
#define VENDOR_MODEL  "1"
#define VENDOR_SW_VER "0.0.1"
#define VENDOR_DATA   NULL
#define JOINER_PSKD   "J01NME"


#define DEFAULT_CHILD_TIMEOUT  40                                           /**< Thread child timeout [s]. */
#define DEFAULT_POLL_PERIOD    1000                                         /**< Thread Sleepy End Device polling period when MQTT-SN Asleep. [ms] */
#define NUM_SLAAC_ADDRESSES    4                                            /**< Number of SLAAC addresses. */
#define OT_JOIN_TRIES          20                                           /**< Amount of attempts to connect to the OT network */


#define SCHED_QUEUE_SIZE       32                                           /**< Maximum number of events in the scheduler queue. */
#define SCHED_EVENT_DATA_SIZE  APP_TIMER_SCHED_EVENT_DATA_SIZE              /**< Maximum app_scheduler event size. */

#define APP_TIM_JOINER_DELAY 200
#define APP_TIMER_TICKS_TIMEOUT APP_TIMER_TICKS(50)

static otNetifAddress m_slaac_addresses[NUM_SLAAC_ADDRESSES];               /**< Buffer containing addresses resolved by SLAAC */

static bool g_sub_registered = false;
static bool g_led_2_on = false;
static bool g_led_3_on = false;

static uint8_t m_ot_join_tries = OT_JOIN_TRIES;                             /**< Down-counter of OT network searching attempts */

/***************************************************************************************************
 * @section scheduler prototypes
 **************************************************************************************************/

static void sched_joiner(void * p_event_data, uint16_t event_size);
static void sched_print_ip(void * p_event_data, uint16_t event_size);
static void sched_mqttsn_gw_search(void * p_event_data, uint16_t event_size);
//static void sched_get_device_data(void * p_event_data, uint16_t event_size);


/***************************************************************************************************
 * @section app prototypes
 **************************************************************************************************/
static void joiner_start(void * p_context);
static void print_ip_info(void);


/***************************************************************************************************
 * @section OpenThread
 **************************************************************************************************/


/**@brief Function for indicating Thread state change.
 */
static void thread_state_changed_callback(uint32_t flags, void * p_context)
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
    thread_state_changed_callback_set(thread_state_changed_callback);
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


/**@brief Callback function for initializing the joiner state.
 */
static void joiner_callback(otError aError, void *aContext)
{
    switch (aError)
    {
        case OT_ERROR_NONE:

            aError = otThreadSetEnabled(thread_ot_instance_get(), true);
            ASSERT(aError == OT_ERROR_NONE);  // TODO is this a good idea?

            NRF_LOG_INFO("Joiner: network found");
            app_sched_event_put(NULL, 0, sched_print_ip);

            m_gateway_search_tries = SEARCH_GATEWAY_TRIES; // TODO find better way
            uint32_t err_code = app_sched_event_put(NULL,
                                                    0,
                                                    sched_mqttsn_gw_search);
            APP_ERROR_CHECK(err_code);

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

        case OT_ERROR_INVALID_STATE:
            NRF_LOG_ERROR("Joiner: failed - invalid state");
        break;

        default:
        break;
    }

    if (aError != OT_ERROR_NONE)
    {
        m_ot_join_tries--;

        if (m_ot_join_tries > 0)
        {
            uint32_t err_code = app_sched_event_put(NULL, 0, sched_joiner);
            APP_ERROR_CHECK(err_code);

            NRF_LOG_INFO("Trying to join the network once again, tries left:%d",
                         m_ot_join_tries - 1);
        }
        else
        {
            NRF_LOG_ERROR("Commissioning failed!\r\nRebooting the device!");
            NRF_LOG_FLUSH();
            NVIC_SystemReset(); // reboot
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
            NRF_LOG_INFO("Joiner: enabled");
        break;

        case OT_ERROR_INVALID_ARGS:
            NRF_LOG_ERROR("Joiner: failed - aPSKd or a ProvisioningUrl is invalid.");
        break;

        case OT_ERROR_DISABLED_FEATURE:
            NRF_LOG_ERROR("Joiner: disabled");
        break;

        default:
        break;
    }
}




/***************************************************************************************************
 * @section MQTT-SN
 **************************************************************************************************/

/*
 * Most of MQTT-SN services is handled by comm_manager module
 */


void mqttsn_init(void)
{
    // register callbacks! !@#$%^&*()_+


    comm_manager_mqttsn_init(thread_ot_instance_get());
}




























/**@brief Function for checking the commissioning status. Triggers the joiner
 * status if the device is not already commissioned.
 */
static void commission_check(void)
{
    if (!otDatasetIsCommissioned(thread_ot_instance_get()))
    {
        NRF_LOG_INFO("Device is not commissioned yet!");

        print_commissioning_info();
        m_ot_join_tries = OT_JOIN_TRIES; // TODO find more elegant way
        app_sched_event_put(NULL, 0, sched_joiner);
    }
    else
    {
        NRF_LOG_INFO("Device successfully commissioned!");
        app_sched_event_put(NULL, 0, sched_print_ip);
    }

    NRF_LOG_PROCESS();
}


/***************************************************************************************************
 * @section State
 **************************************************************************************************/

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


/**@brief Function for initializing scheduler module.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}


static void sched_joiner(void * p_event_data, uint16_t event_size)
{
    joiner_start(NULL);
}

static void sched_print_ip(void * p_event_data, uint16_t event_size)
{
    print_ip_info();
}

static void sched_mqttsn_gw_search(void * p_event_data, uint16_t event_size)
{
    comm_manager_search_gateway();
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
    commission_check();
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
