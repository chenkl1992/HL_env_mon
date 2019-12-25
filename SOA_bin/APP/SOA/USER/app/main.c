/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "cpu.h"
#include  "lib_mem.h"
#include  "os.h"
#include  "clk.h"
#include  "bsp_os.h"
#include  "bsp_clk.h"
#include  "bsp_led.h"
#include  "bsp_int.h"
#include  "stm32f1xx_hal.h"

#include  "os_app_hooks.h"
#include  "app_cfg.h"

//#include  "debug.h"
#include "printf-stdarg.h"
#include "sFlash.h"
#include "sds.h"

#include  "hl_msgdeal.h"
#include  "net_service.h"
#include  "tcp_service.h"
#include  "udp_service.h"
#include  "data_service.h"
#include  "register_service.h"
#include  "heartbeat_service.h"
#include  "DA_service.h"
#include  "event_service.h"
#include  "service_shell.h"
#include  "supervision_service.h"
#include  "update_service.h"

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  StartupTask (void  *p_arg);


static  OS_TCB   StartupTaskTCB;
static  CPU_STK  StartupTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE];


/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : none
*********************************************************************************************************
*/

int  main (void)
{
#warning 升级  
    SCB->VTOR = FLASH_BASE | 0x8800;
    OS_ERR  os_err;

    HAL_Init();                                                 /* Initialize STM32Cube HAL Library                     */
    BSP_ClkInit();                                              /* Initialize the main clock                            */
    BSP_OS_TickInit();                                          /* Initialize kernel tick timer                         */

    Mem_Init();                                                 /* Initialize Memory Managment Module                   */
    CPU_IntDis();                                               /* Disable all Interrupts                               */
    CPU_Init();                                                 /* Initialize the uC/CPU services                       */

    OSInit(&os_err);                                            /* Initialize uC/OS-III                                 */
    if (os_err != OS_ERR_NONE) {
        while (1);
    }

    App_OS_SetAllHooks();                                       /* Set all applications hooks                           */

    OSTaskCreate((OS_TCB     *)&StartupTaskTCB,                               /* Create the startup task                              */
                 (CPU_CHAR   *)"Startup Task",
                 (OS_TASK_PTR )StartupTask,
                 (void       *)0u,
                 (OS_PRIO     )APP_CFG_STARTUP_TASK_PRIO,
                 (CPU_STK    *)&StartupTaskStk[0u],
                 (CPU_STK_SIZE)APP_CFG_STARTUP_TASK_STK_SIZE / 10u,
                 (CPU_STK_SIZE)APP_CFG_STARTUP_TASK_STK_SIZE,
                 (OS_MSG_QTY  )0u,
                 (OS_TICK     )0u,
                 (void       *)0u,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
    if (os_err != OS_ERR_NONE) {
        while (1);
    }

    OSStart(&os_err);                                           /* Start multitasking (i.e. give control to uC/OS-III)  */

    while (DEF_ON) {                                            /* Should Never Get Here.                               */
        ;
    }
}


/*
*********************************************************************************************************
*                                            STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'StartupTask()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  StartupTask (void *p_arg)
{
    OS_ERR  os_err;

   (void)p_arg;


    OS_TRACE_INIT();                                            /* Initialize the OS Trace recorder                     */

    BSP_OS_TickEnable();                                        /* Enable the tick timer and interrupt                  */

    BSP_LED_Init();                                             /* Initialize LEDs                                      */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&os_err);                            /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif

#if OS_CFG_SCHED_ROUND_ROBIN_EN > 0u
	OSSchedRoundRobinCfg(DEF_ENABLED,0,&os_err);
#endif
	
	CLK_ERR  clk_err;
	Clk_Init(&clk_err);
	
#if CLK_CFG_EXT_EN == DEF_DISABLED
	CLK_DATE_TIME  date_time;
	Clk_DateTimeMake(&date_time, 2019, 1, 1, 0, 0, 0, 0);
	Clk_SetDateTime(&date_time);
#endif
	
	debug_init();
        printf("APP start\r\n");
	sFlash_Init();
        SDS_Init();

	MsgDeal_Init(); /* 初始化消息处理模块 */
        software_versionInit();
        service_shell_init(NULL);
        start_data_service();
        start_net_service();  	/* 网络服务 */
        start_supervision_service();
        start_register_service();
        start_heartbeat_service();
        start_update_service();
        start_udp_service();
        start_tcp_service();
        start_event_service();
        start_DA_service();
        
	
    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
        OSTimeDlyHMSM(0u, 0u, 1u, 0u,OS_OPT_TIME_HMSM_STRICT,&os_err);
    }
}