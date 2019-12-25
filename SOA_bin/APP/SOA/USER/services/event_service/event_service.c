#include  "stm32f1xx_hal.h"
#include  "app_cfg.h"
#include  "lwip.h"
#include  "lwip/api.h"
#include  <assert.h>
#include  "hl_msgdeal.h"
#include  "data_service.h"
#include  "tcp_service.h"
#include  "event_service.h"
#include  "net_service.h"
#include  "DA_service.h"
#include  "supervision_service.h"
#include  "string.h"
#include  "log.h"

/* 报警 服务所需要的资源 */
static  OS_Q  		event_service_q;
static  OS_Q  		event_service_ack_q;
static  void  		event_service (void  *p_arg);
static  OS_TCB   	event_service_tcb;
static  CPU_STK  	event_service_stk[APP_CFG_EVENT_TASK_STK_SIZE];

OS_TMR event_tmr;
static EventConf_t EventConf[DEV_NUM] = {0};
static uint16_t my_seq_id = 0;

OS_Q*  get_event_service_handler(void)
{
  return &event_service_q;
}

OS_Q*  get_event_service_ack_handler(void)
{
  return &event_service_ack_q;
}

OS_TCB get_event_service_tcb(void)
{
  return event_service_tcb;
}


int event_ack_process(uint8_t* buf, Sensor_State_t Sensor_State, uint8_t type)
{
  uint16_t seqID;
  uint8_t pid;
  memcpy(&seqID, buf, sizeof(uint16_t));
  pid = buf[2];
    //判断命令序列号
  if(my_seq_id == seqID)
  {
    if(Sensor_State.errorFlag == SET)
    {
      if(pid == 0x0F)
      {
        if(EventConf[type].errorStartFlag == SET)
        {
          EventConf[type].errorStartFlag = RESET;
          return 0;
        }
      }
    }
    if(Sensor_State.errorFlag == RESET)
    {
      if(pid == 0x10)
      {
        if(EventConf[type].errorStartFlag == SET)
        {
          EventConf[type].errorStartFlag = RESET;
          return 0;
        }
      }
    }
    if(Sensor_State.eventFlag == SET)
    {
      if(pid == 0x0D)
      {
        Sensor_State_t Sensor_State = {0};
        Sensor_State.eventFlag = RESET;
        Sensor_State.type = type;
        set_sys_sensor_State(&Sensor_State, 0);
        if(EventConf[type].eventStartFlag == SET)
        {
          EventConf[type].eventStartFlag = RESET;
          return 0;
        }
      }
    }
    if(Sensor_State.eventFlag == RECOVER)
    {
      if(pid == 0x0E)
      {
        Sensor_State_t Sensor_State = {0};
        Sensor_State.eventFlag = RESET;
        Sensor_State.type = type;
        set_sys_sensor_State(&Sensor_State, 0);
        if(EventConf[type].eventStartFlag == SET)
        {
          EventConf[type].eventStartFlag = RESET;
          return 0;
        }
      }
    }
  }
  return 1;
}

void event_stop(void)
{
  SysConf_t config = {0};
  get_config(&config);
  for(uint8_t i=0; i<DEV_NUM; i++)
  {
    if(config.Sensor[i].enFlag == RESET)
    {
      EventConf[i].errorStartFlag = RESET;
      EventConf[i].eventStartFlag = RESET;
    }
  }
}

/*  
 * return 0  : 收到对应的应答
 * return -1 : 收到超时通知
 * return -2 : 收到应答，但不是本次的应答
 */
static int MsgAckHandle(MSG_BUF_t* recvbuf, Sensor_State_t Sensor_State, uint8_t type)
{
  int ret = 0xFF;
  if(recvbuf->mtype == MSG_ID_EVENT_ACK)
  {
     ret = event_ack_process(recvbuf->buf, Sensor_State, type);
  }
  return ret;
}

static void wait_event_ack(Sensor_State_t Sensor_State, uint8_t type)
{
  int ret = 0x01;
  MSG_BUF_t* recv_buf = NULL;
  OS_Q* pQ = get_event_service_ack_handler();
  MsgDeal_Clr(pQ);
  for(int i=0; i<3; i++) /*等待三次，最多3秒 */
  {
    if(MsgDeal_Recv(pQ,&recv_buf, 1000) == 0)
    {
      ret = MsgAckHandle(recv_buf, Sensor_State, type);
      MsgDeal_Free(recv_buf);
    }
    if(ret == 0)
    {
      break;
    }
  }
  if(ret == 0)
  {
    loginf("Event ack success\r\n");
  }
}

static void event_data_report(void)
{
  if(get_device_state() == DEVICE_STATE_WORK)
  {
    for(uint8_t i=0; i<DEV_NUM; i++)
    {
      if(EventConf[i].errorStartFlag == SET)
      {
        send_error_data(&(EventConf[i].Sensor_State), &my_seq_id);
        wait_event_ack(EventConf[i].Sensor_State, i);
      }
      if(EventConf[i].eventStartFlag == SET)
      {
        send_event_data(&(EventConf[i].Sensor_State), EventConf[i].Sensor_State.value, &my_seq_id);
        wait_event_ack(EventConf[i].Sensor_State, i);
      }
    }
  }
  /*继续*/
  OS_ERR err;
  OSTmrStart (&event_tmr,&err);
  if(err != OS_ERR_NONE)
    logerr("event service tmr restart fail !\r\n");
  //logdbg("event failed restart\r\n");
}

void event_alarm(uint8_t* buf)
{
  Sensor_State_t Sensor_State = {0};
  memcpy(&Sensor_State, buf, sizeof(Sensor_State_t));
  EventConf[Sensor_State.type].eventStartFlag = SET;
  memcpy(&(EventConf[Sensor_State.type].Sensor_State), &Sensor_State, sizeof(Sensor_State_t));
}

void event_fault(uint8_t* buf)
{
  Sensor_State_t Sensor_State = {0};
  memcpy(&Sensor_State, buf, sizeof(Sensor_State_t));
  EventConf[Sensor_State.type].errorStartFlag = SET;
  memcpy(&(EventConf[Sensor_State.type].Sensor_State), &Sensor_State, sizeof(Sensor_State_t));
}

static void handle_msg(MSG_BUF_t* msg)
{ 
  switch(msg->mtype)
  {
  case MSG_ID_EVENT_ALARM:
    event_alarm(msg->buf);
    loginf("event alarm\r\n");
    break;
  case MSG_ID_EVENT_FAULT:
    event_fault(msg->buf);
    loginf("event fault\r\n");
    break; 
  case MSG_ID_EVENT_STOP:
    event_stop();
    loginf("Event Stop\r\n"); 
    break;
  case MSG_ID_EVENT_TIMEOUT:
    event_data_report();
    break;
  default:
    logerr("EVENT service ID err\r\n");
    break;
  }
}

void event_data_init(void)
{
  //读取 掉电保存的 报警标志位
  SysConf_t config = {0};
  get_config(&config);
  for(uint8_t i=0; i<DEV_NUM; i++)
  {
    if(config.Sensor_State[i].eventFlag != RESET)
    {
      EventConf[i].eventStartFlag = SET;
      memcpy(&(EventConf[i].Sensor_State), (&config.Sensor_State[i]), sizeof(Sensor_State_t));
    }
  }
}

void event_tmr_callback(void *p_tmr, void *p_arg)
{
  if(MsgDeal_Send(NULL, get_event_service_handler(), MSG_ID_EVENT_TIMEOUT, NULL, 0) != 0)
  {
    logerr("event_tmre send to event service fail !\r\n");
  }
}

void start_event_service(void)
{
  OS_ERR  os_err;
  OSQCreate (&event_service_q,"event_service_q",5,&os_err);
  assert (os_err == OS_ERR_NONE);
  
  OSQCreate (&event_service_ack_q,"event_service_ack_q",5,&os_err);
  assert (os_err == OS_ERR_NONE);
  
  OSTmrCreate((OS_TMR               *)&event_tmr,	//定时器分辨率100ms 
               (CPU_CHAR             *)"event_tmr",
               (OS_TICK               )50,		//初始化延迟 50*100ms = 5s
               (OS_TICK               )50,		//周期 50*100ms = 5s
               (OS_OPT                )OS_OPT_TMR_ONE_SHOT,	//有初始化延迟的周期定时器模式
               (OS_TMR_CALLBACK_PTR   )event_tmr_callback,
               (void                 *)0,
               (OS_ERR               *)&os_err);
  assert (os_err == OS_ERR_NONE);
  

  assert (os_err == OS_ERR_NONE);
  OSTaskCreate((OS_TCB     *)&event_service_tcb,                               /* Create the startup task                              */
               (CPU_CHAR   *)"event service",
               (OS_TASK_PTR )event_service,
               (void       *)0u,
               (OS_PRIO     )APP_CFG_EVENT_TASK_PRIO,
               (CPU_STK    *)&event_service_stk[0u],
               (CPU_STK_SIZE)APP_CFG_EVENT_TASK_STK_SIZE / 10u,
               (CPU_STK_SIZE)APP_CFG_EVENT_TASK_STK_SIZE,
               (OS_MSG_QTY  )0u,
               (OS_TICK     )0u,
               (void       *)0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR     *)&os_err);
  assert (os_err == OS_ERR_NONE) ;
}

/* 服务 */
static  void  event_service(void *p_arg)
{
  MSG_BUF_t* recv_info  = NULL;
  OS_ERR  os_EVENT_err;
  event_data_init();
  OSTmrStart(&event_tmr, &os_EVENT_err);//开启定时器
  while(1)
  {
    if(MsgDeal_Recv(get_event_service_handler(), &recv_info, 5000) == 0)
    {
      handle_msg(recv_info);
      MsgDeal_Free(recv_info);
    }
    set_task_alive_state(EVENT_SERVICE_TASK);
  }
}
