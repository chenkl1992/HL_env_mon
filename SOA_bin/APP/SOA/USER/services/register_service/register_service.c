#include  "stm32f1xx_hal.h"
#include  "app_cfg.h"
#include  "lwip.h"
#include  "string.h"
#include  "lwip/api.h"
#include  <assert.h>
#include  "hl_msgdeal.h"
#include  "register_service.h"
#include  "heartbeat_service.h"
#include  "tcp_service.h"
#include  "data_service.h"
#include  "net_service.h"
#include  "supervision_service.h"
#include  "DA_service.h"
#include  "os.h"
#include  "log.h"
/*  设备工作模式 */


/* tcp 服务所需要的资源 */
static  OS_Q  		register_service_q;
static  void  		register_service (void  *p_arg);
static  OS_TCB   	register_service_tcb;
static  CPU_STK  	register_service_stk[APP_CFG_REGISTER_TASK_STK_SIZE];

/* 注册消息应答队列 */
static  OS_Q  		register_ack_q;

void register_tmr_callback(void *p_tmr, void *p_arg);
static uint16_t my_seq_id = 0;
uint8_t logoutFlag = RESET;


OS_ERR  reg_err;
OS_TMR register_tmr;

OS_Q*  get_register_service_handler(void)
{
  return &register_service_q;
}

OS_TCB get_regist_service_tcb(void)
{
  return register_service_tcb;
}

OS_Q*  get_register_service_ack_handler(void)
{
  return &register_ack_q;
}

uint8_t get_logout_flag(void)
{
  return logoutFlag;
}

/*  
 * return 0  : 收到对应的应答
 * return -1 : 收到超时通知
 * return -2 : 收到应答，但不是本次的应答
 */
static int MsgAckHandle(MSG_BUF_t* recvbuf)
{
  int ret = 0xFF;
  if(recvbuf->mtype == MSG_ID_LOG_IN_ACK)
  {
    uint16_t sn = 0;
    assert(recvbuf->len == 2);
    char *pbuf = (char *)recvbuf->buf;
    sn = (uint16_t)(pbuf[1] << 8 | pbuf[0]);   
    if(sn != my_seq_id)
      ret = -2;
    else
      ret = 0;
  }
  return ret;
}

/* 注册请求 */
static void msg_id_dev_login_ind(uint8_t* buf)
{
  OS_ERR err;
  int ret = 0xFF;
  MSG_BUF_t* recv_buf = NULL;
  OS_Q* pQ = get_register_service_ack_handler();
  
  MsgDeal_Clr(pQ);
#warning sleep after sensor work  
  if(DA_Init_flag == SET)
  {
    loginf("regist\r\n");
    send_login_info(buf, &my_seq_id);
    for(int i=0; i<3; i++) /*等待三次，最多三秒 */
    {
      if(MsgDeal_Recv(pQ,&recv_buf,1000) == 0)
      {
        ret = MsgAckHandle(recv_buf);
        MsgDeal_Free(recv_buf);
      }
      if(ret == 0)
      {
        break;
      }
    }
  }
  if(ret != 0)
  {
    //if(get_device_state() == DEVICE_STATE_STAND_BY)
    {
      /* 继续注册 */
      OSTmrStart (&register_tmr,&err);
      if(err != OS_ERR_NONE)
        logerr("register service tmr restart fail !\r\n");
      logdbg("register failed restart %2x\r\n", ret);
    }
  }
  else
  {
    set_device_state(DEVICE_STATE_WORK);
    if(MsgDeal_Send(get_register_service_handler(), get_heartbeat_service_handler(), MSG_ID_START_HEARTBEAT, NULL, 0) != 0)
    {
      logerr("register_tmr send to register service fail !\r\n");
    }
    loginf("register success\r\n");
  }
}

/* 注销请求 */
static void msg_id_dev_logout_ind(uint8_t* buf)
{
  send_logout_info(buf);
  loginf("Logout\r\n");
  logoutFlag = SET;  
  if(MsgDeal_Send(get_register_service_handler(), get_tcp_service_handler(), MSG_ID_UPGRADE_RESET, NULL, 0) != 0)
  {
    logerr("register send to tcp service fail !\r\n");
  }
}

static void handle_msg(MSG_BUF_t* msg)
{
  switch(msg->mtype)
  {
  case MSG_ID_LOG_IN:
    msg_id_dev_login_ind(msg->buf);
    break;
  case MSG_ID_LOG_OUT:
    msg_id_dev_logout_ind(msg->buf);
    break;
  default:
    logerr("Register ID err\r\n");
    break;
  }
}
/* 注册、注销 服务 */
void start_register_service(void)
{
  OSQCreate (&register_ack_q,"register_ack_q",5,&reg_err);
  assert (reg_err == OS_ERR_NONE);
  
  OSQCreate (&register_service_q, "register_service_q",5, &reg_err);
  assert (reg_err == OS_ERR_NONE);
  
  OSTaskCreate((OS_TCB     *)&register_service_tcb,                               /* Create the startup task                              */
               (CPU_CHAR   *)"register service",
               (OS_TASK_PTR )register_service,
               (void       *)0u,
               (OS_PRIO     )APP_CFG_REGIST_TASK_PRIO,
               (CPU_STK    *)&register_service_stk[0u],
               (CPU_STK_SIZE)APP_CFG_REGISTER_TASK_STK_SIZE / 10u,
               (CPU_STK_SIZE)APP_CFG_REGISTER_TASK_STK_SIZE,
               (OS_MSG_QTY  )0u,
               (OS_TICK     )0u,
               (void       *)0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR     *)&reg_err);
  assert (reg_err == OS_ERR_NONE);
}

/* 注册 服务 */
static void register_service(void *p_arg)
{
  MSG_BUF_t* recv_info  = NULL;
  OSTmrCreate((OS_TMR               *)&register_tmr,	//定时器分辨率100ms 
              (CPU_CHAR             *)"register_tmr",
              (OS_TICK               )100,		//初始化延迟 100*100ms = 10s
              (OS_TICK               )100,		//周期 100*100ms = 10s
              (OS_OPT                )OS_OPT_TMR_ONE_SHOT,	//有初始化延迟的周期定时器模式
              (OS_TMR_CALLBACK_PTR   )register_tmr_callback,
              (void                 *)0,
              (OS_ERR               *)&reg_err);
  assert (reg_err == OS_ERR_NONE);
  
  while(1)
  {	
    if(MsgDeal_Recv(&register_service_q, &recv_info, 5000) == 0)
    {
      handle_msg(recv_info);
      MsgDeal_Free(recv_info);
    }
    set_task_alive_state(REGISTER_SERVICE_TASK);
  }			
}


void register_tmr_callback(void *p_tmr, void *p_arg)
{
  uint8_t cause;
  cause = CAUSE_NO_RESPONSE;
  if(MsgDeal_Send(NULL, get_register_service_handler(), MSG_ID_LOG_IN, &cause, 1) != 0)
  {
    logerr("register_tmr send to register service fail !\r\n");
  }
}