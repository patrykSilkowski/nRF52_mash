/*
 * service_bsp.h
 *
 *  Created on: Jan 22, 2019
 *      Author: MSc Patryk Silkowski
 */

#ifndef APP_SERVICE_BSP_H_
#define APP_SERVICE_BSP_H_


#define SERVICE_BSP_LED0      0
#define SERVICE_BSP_LED1      1
#define SERVICE_BSP_LED2      2
#define SERVICE_BSP_LED3      3
#define SERVICE_BSP_SW0       4
#define SERVICE_BSP_SW1       5
#define SERVICE_BSP_SW2       6
#define SERVICE_BSP_SW3       7

#define SERVICE_BSP_ENDPOINTS 8

/*
 * The modification of the following enum is critical!
 * A lot of stuff uses endpoint_none value to indicate the number
 * of endpoints on this particular platform/board
 */


#endif /* APP_SERVICE_BSP_H_ */
