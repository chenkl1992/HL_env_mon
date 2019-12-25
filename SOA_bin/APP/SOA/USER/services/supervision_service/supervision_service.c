#include  "stm32f1xx_hal.h"
#include  "app_cfg.h"
#include  "lwip.h"
#include  "lwip/api.h"
#include  <assert.h>
#include  "hl_msgdeal.h"
#include  "data_service.h"
#include  "update_service.h"
#include  "supervision_service.h"
#include  "tcp_service.h"
#include  "udp_service.h"
#include  "register_service.h"
#include  "heartbeat_service.h"
#include  "event_service.h"
#include  "DA_service.h"
#include  "net_service.h"

#include  "log.h"


/* 服务所需要的资源 */
static  OS_Q  		supervision_service_q;
static  void  		supervision_service (void  *p_arg);
static  OS_TCB   	supervision_service_tcb;
static  CPU_STK  	supervision_service_stk[APP_CFG_SUPERVISION_TASK_STK_SIZE];

OS_MUTEX SVPlock;
task_state_t task_alive_list[SERVICE_NUM] = {0};
IWDG_HandleTypeDef hiwdg;
//static  OS_TCB          service_tcb_list;

void set_task_alive_state(uint8_t serviceNum)
{
  task_alive_list[serviceNum].current++;
}

void start_supervision_service(void)
{
  OS_ERR  os_err;
  OSMutexCreate(&SVPlock, "SVP", &os_err);
  assert(os_err == OS_ERR_NONE);
  
  OSQCreate (&supervision_service_q,"supervision_service_q",10,&os_err);
  assert (os_err == OS_ERR_NONE);
  
  OSTaskCreate((OS_TCB     *)&supervision_service_tcb,                               /* Create the startup task                              */
               (CPU_CHAR   *)"supervision service",
               (OS_TASK_PTR )supervision_service,
               (void       *)0u,
               (OS_PRIO     )APP_CFG_SUPERVISION_TASK_PRIO,
               (CPU_STK    *)&supervision_service_stk[0u],
               (CPU_STK_SIZE)APP_CFG_SUPERVISION_TASK_STK_SIZE / 10u,
               (CPU_STK_SIZE)APP_CFG_SUPERVISION_TASK_STK_SIZE,
               (OS_MSG_QTY  )0u,
               (OS_TICK     )0u,
               (void       *)0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR     *)&os_err);
  assert (os_err == OS_ERR_NONE);
}

uint8_t is_task_alive(uint8_t num)
{
  if(task_alive_list[num].before != task_alive_list[num].current)
  {
    task_alive_list[num].not_alive_counter = 0;
    task_alive_list[num].before = task_alive_list[num].current;
  }
  else
  {
    if(task_alive_list[num].not_alive_counter < 3)
    {
      task_alive_list[num].not_alive_counter++;
    }
  }
  if(task_alive_list[num].not_alive_counter > 2)
  {
    return 0; 
  }
  return 1;
}

void is_all_task_alive(void)
{
  for(uint8_t i=0; i<SERVICE_NUM; i++)
  {
    if(!is_task_alive(i))
    {
      logerr("task :%d not alive\r\n", i);
    }
  }
}

void is_cpu_overload(void)
{
  //uint8_t device_state;
  //device_state = get_device_state();
  //loginf("device_state %d\r\n", device_state);  
  loginf("CPU Usage: %d\r\n", OSStatTaskCPUUsage);
}

void IWDG_Init(void)
{
  //40k/256*n = feed timeout
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
  hiwdg.Init.Reload = 2000;
  HAL_IWDG_Init(&hiwdg);
}

/* 服务 */
static  void  supervision_service (void *p_arg)
{
  OS_ERR  err;
  IWDG_Init();
  while(1)
  {
    is_all_task_alive();
    is_cpu_overload();
    HAL_IWDG_Refresh(&hiwdg);
    OSTimeDly (10000,OS_OPT_TIME_DLY,&err);
  }
}
