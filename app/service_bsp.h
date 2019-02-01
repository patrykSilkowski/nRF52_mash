/*
 * service_bsp.h
 *
 *  Created on: Jan 22, 2019
 *      Author: MSc Patryk Silkowski
 */

#ifndef APP_SERVICE_BSP_H_
#define APP_SERVICE_BSP_H_


#define SERVICE_BSP_ENDPOINTS 8

/*
 * The modification of the following enum is critical!
 * A lot of stuff uses endpoint_none value to indicate the number
 * of endpoints on this particular platform/board
 */
typedef enum {
    button_0 = 0,
    button_1,
    button_2,
    button_3,
    led_0,
    led_1,
    led_2,
    led_3,
    endpoint_none
} endpoint_t;


#endif /* APP_SERVICE_BSP_H_ */
