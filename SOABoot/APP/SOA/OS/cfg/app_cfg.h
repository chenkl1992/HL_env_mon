/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                      APPLICATION CONFIGURATION
*
* Filename : app_cfg.h
*********************************************************************************************************
*/

#ifndef  _APP_CFG_H_
#define  _APP_CFG_H_


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <stdarg.h>
#include  <stdio.h>
#include  "cpu_cfg.h"


/*
*********************************************************************************************************
*                                       MODULE ENABLE / DISABLE
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           TASK PRIORITIES
*********************************************************************************************************
*/

#define  OS_TASK_TMR_PRIO                (OS_LOWEST_PRIO - 2u)  /* Only required for uC/OS-II                           */

#define  APP_CFG_STARTUP_TASK_PRIO	                         10u

#define  APP_CFG_NET_TASK_PRIO		                         2u
#define  APP_CFG_TCP_TASK_PRIO		                         3u
#define  APP_CFG_UDP_TASK_PRIO		                         4u
#define  APP_CFG_REGIST_TASK_PRIO		                 8u
#define  APP_CFG_HEARTBEAT_TASK_PRIO		                 9u

#define  APP_CFG_DATA_TASK_PRIO		                         5u
#define  OS_PRO_SERVICE_SHELL		                         6u
#define  APP_CFG_UPDATE_TASK_PRIO		                 11u
#define  APP_CFG_REPORT_TASK_PRIO		                 7u
#define  APP_CFG_EVENT_TASK_PRIO                                 12u
#define  APP_CFG_DA_TASK_PRIO                                    13u
#define  APP_CFG_SUPERVISION_TASK_PRIO                           14u

/*
*********************************************************************************************************
*                                          TASK STACK SIZES
*                             Size of the task stacks (# of OS_STK entries)
*********************************************************************************************************
*/

#define  APP_CFG_STARTUP_TASK_STK_SIZE				256u

#define  APP_CFG_NET_TASK_STK_SIZE           		        128u
#define  APP_CFG_REGISTER_TASK_STK_SIZE           		128u
#define  APP_CFG_HEARTBEAT_TASK_STK_SIZE           		128u
#define  APP_CFG_TCP_TASK_STK_SIZE           		        256u
#define  APP_CFG_UDP_TASK_STK_SIZE           		        256u
#define APP_CFG_SUPERVISION_TASK_STK_SIZE                       128u

#define  OS_STK_SIZE_SERVICE_SHELL                              128u
#define  APP_CFG_DATA_TASK_STK_SIZE           		        256u
#define  APP_CFG_UPDATE_TASK_STK_SIZE		                256u
#define  APP_CFG_REPORT_TASK_STK_SIZE		                256u
#define  APP_CFG_EVENT_TASK_STK_SIZE		                256u
#define  APP_CFG_DA_TASK_STK_SIZE		                256u

/*
*********************************************************************************************************
*                                     TRACE / DEBUG CONFIGURATION
*********************************************************************************************************
*/

#define  APP_TRACE_LEVEL                   TRACE_LEVEL_OFF
#define  APP_TRACE                         printf

#define  APP_TRACE_INFO(x)    ((APP_TRACE_LEVEL >= TRACE_LEVEL_INFO)  ? (void)(APP_TRACE x) : (void)0)
#define  APP_TRACE_DBG(x)     ((APP_TRACE_LEVEL >= TRACE_LEVEL_DBG)   ? (void)(APP_TRACE x) : (void)0)


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif                                                          /* End of module include.              */
