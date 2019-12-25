#include  "stm32f1xx_hal.h"
#include  "app_cfg.h"
#include  "lwip.h"
#include  "log.h"
#include  "lwip/api.h"
#include  <assert.h>
#include  "hl_msgdeal.h"
#include  "heartbeat_service.h"
#include  "tcp_service.h"
#include  "data_service.h"
#include  "net_service.h"
#include  "DA_service.h"
#include  "supervision_service.h"
#include  "register_service.h"
#include  "os.h"


/* 服务所需要的资源 */
static  OS_Q  		heartbeat_service_q;
static  void  		heartbeat_service (void  *p_arg);
static  OS_TCB   	heartbeat_service_tcb;
static  CPU_STK  	heartbeat_service_stk[APP_CFG_HEARTBEAT_TASK_STK_SIZE];

void heartbeat_tmr_callback(void *p_tmr, void *p_arg);
uint8_t heartbeat_tmr_StartFlag = RESET;

OS_ERR  os_HB_err;
OS_TMR heartbeat_tmr;

OS_Q*  get_heartbeat_service_handler(void)
{
  return &heartbeat_service_q;
}

OS_TCB get_heartbeat_service_tcb(void)
{
  return heartbeat_service_tcb;
}

static void heartbeat_timeout_process(void)
{
  //心跳 默认30s一次
  //状态改变
//  if(get_device_state() != DEVICE_STATE_WORK)
//  {
//    //关定时器
//    if(heartbeat_tmr_StartFlag == SET)
//    {  
//      OSTmrStop(&heartbeat_tmr,OS_OPT_TMR_NONE,0,&os_HB_err);
//      heartbeat_tmr_StartFlag = RESET;
//      loginf("HeartBeat Timer close State change\r\n");
//    }
//  }
  if(get_device_state() == DEVICE_STATE_WORK)
  {
    if(DA_Init_flag == SET)
    {
      send_heart_beat();
    }
    set_heartbeat_counter();
    if(get_heartbeat_counter() > 2)
    {
      set_device_state(DEVICE_STATE_STAND_BY);
      loginf("Heart No recive\r\n");
      uint8_t log_cause;
      log_cause = CAUSE_NO_RESPONSE;
      if(MsgDeal_Send(get_tcp_service_handler(), get_register_service_handler(), MSG_ID_LOG_IN, &log_cause, 1) != 0)
      {
        loginf("heart beat send to register service fail !\r\n");
      }
    }
    loginf("Heart TimeOut\r\n");
  } 
}

void start_heartbeat(void)
{
  set_heartbeat_zero();
  //send_heart_beat();
  if(heartbeat_tmr_StartFlag == RESET)
  {
    heartbeat_tmr_StartFlag = SET;
    OSTmrStart(&heartbeat_tmr,&os_HB_err);//开启定时器
    loginf("Heart Beat TimerStart\r\n");
  }
}

void heartbeat_dur_change(void)
{
  OSTmrStop(&heartbeat_tmr,OS_OPT_TMR_NONE,0,&os_HB_err);
  OSTmrSet((OS_TMR               *)&heartbeat_tmr,
           (OS_TICK               )0,
           (OS_TICK               )sysconfig.report_interval*10,
           (OS_TMR_CALLBACK_PTR   )heartbeat_tmr_callback,
           (void                 *)0,
           (OS_ERR               *)&os_HB_err);
  OSTmrStart(&heartbeat_tmr,&os_HB_err);
  loginf("Heart Beat Timer Change: %d\r\n", sysconfig.report_interval);
}

static void handle_msg(MSG_BUF_t* msg)
{
  //根据msg id 发送msg cause
  switch(msg->mtype)
  {
  case MSG_ID_START_HEARTBEAT:
    start_heartbeat();
    break;
  case MSG_ID_HEARTBEAT_DURATION_CHANGE:
    heartbeat_dur_change();
    break;
  case MSG_ID_HEARTBEAT_TIMEOUT:
    heartbeat_timeout_process();
    break;
  default:
    logerr("Heartbeat ID err\r\n");
    break;
  }
}
/* 心跳 服务 */
void start_heartbeat_service(void)
{
  OSTmrCreate((OS_TMR               *)&heartbeat_tmr,	                                //定时器分辨率100ms 
              (CPU_CHAR             *)"heartbeat_tmr",
              (OS_TICK               )sysconfig.report_interval*10,		//初始化延迟 300*100ms = 30s
              (OS_TICK               )sysconfig.report_interval*10,		//周期 300*100ms = 30s
              (OS_OPT                )OS_OPT_TMR_PERIODIC,	                //有初始化延迟的周期定时器模式
              (OS_TMR_CALLBACK_PTR   )heartbeat_tmr_callback,
              (void                 *)0,
              (OS_ERR               *)&os_HB_err);
  assert (os_HB_err == OS_ERR_NONE);
  OSQCreate (&heartbeat_service_q,"heartbeat_service_q",10,&os_HB_err);
  
  assert (os_HB_err == OS_ERR_NONE);
  OSTaskCreate((OS_TCB     *)&heartbeat_service_tcb,                               /* Create the startup task                              */
               (CPU_CHAR   *)"heartbeat service",
               (OS_TASK_PTR )heartbeat_service,
               (void       *)0u,
               (OS_PRIO     )APP_CFG_HEARTBEAT_TASK_PRIO,
               (CPU_STK    *)&heartbeat_service_stk[0u],
               (CPU_STK_SIZE)APP_CFG_HEARTBEAT_TASK_STK_SIZE / 10u,
               (CPU_STK_SIZE)APP_CFG_HEARTBEAT_TASK_STK_SIZE,
               (OS_MSG_QTY  )0u,
               (OS_TICK     )0u,
               (void       *)0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR     *)&os_HB_err);
  assert (os_HB_err == OS_ERR_NONE);
}

/* 心跳 服务 */
static void heartbeat_service(void *p_arg)
{		
  MSG_BUF_t* recv_info  = NULL;
  while(1)
  {	
    if(MsgDeal_Recv(get_heartbeat_service_handler(),&recv_info, 5000) == 0)
    {
      handle_msg(recv_info);
      MsgDeal_Free(recv_info);
    }
    set_task_alive_state(HB_SERVICE_TASK);
  }			
}

void heartbeat_tmr_callback(void *p_tmr, void *p_arg)
{
  if(MsgDeal_Send(NULL, get_heartbeat_service_handler(), MSG_ID_HEARTBEAT_TIMEOUT, NULL, 0) != 0)
  {
    loginf("tm2  send to heartbeat service fail !\r\n");
  }
}