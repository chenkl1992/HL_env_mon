#include  "stm32f1xx_hal.h"
#include  "app_cfg.h"
#include  "lwip.h"
#include  "lwip/api.h"
#include  <assert.h>
#include  "hl_msgdeal.h"
#include  "data_service.h"
#include  "update_service.h"
#include  "net_service.h"
#include  "tcp_service.h"
#include  "supervision_service.h"
#include  "log.h"
/*  设备工作模式 */


/* 网络 服务所需要的资源 */
static  OS_Q  		net_service_q;
static  void  		net_service (void  *p_arg);
static  OS_TCB   	net_service_tcb;
static  CPU_STK  	net_service_stk[APP_CFG_NET_TASK_STK_SIZE];

OS_MUTEX NETlock;
volatile static uint8_t device_state = DEVICE_STATE_NOT_RDY;

OS_Q*  get_net_service_handler(void)
{
  return &net_service_q;
}

OS_TCB get_net_service_tcb(void)
{
  return net_service_tcb;
}

//设备状态
uint8_t get_device_state(void)
{
  uint8_t dev_state;
  OS_ERR  os_err;
  OSMutexPend(&NETlock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  dev_state = device_state;
  OSMutexPost(&NETlock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
  return dev_state;
}

void set_device_state(uint8_t dev_state)
{
  OS_ERR  os_err;
  OSMutexPend(&NETlock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  device_state = dev_state;
  OSMutexPost(&NETlock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
}

void lwip_reset_netif_ipaddr(void)
{
  lwip_dev_t dev = {0};
  get_lwip_dev(&dev);
  ip_addr_t  ip;
  ip_addr_t  gateway, netmask;
  IP4_ADDR(&ip, dev.ip[0],dev.ip[1],dev.ip[2], dev.ip[3]);
  IP4_ADDR(&netmask, dev.netmask[0],dev.netmask[1],dev.netmask[2], dev.netmask[3]);
  IP4_ADDR(&gateway, dev.gateway[0],dev.gateway[1],dev.gateway[2], dev.gateway[3]);
  netif_set_down(&gnetif);
  netif_set_ipaddr(&gnetif, &ip);
  netif_set_netmask(&gnetif, &netmask);
  netif_set_gw(&gnetif, &gateway); 
  netif_set_up(&gnetif);
}

static void handle_msg(MSG_BUF_t* msg)
{
  switch(msg->mtype)
  {
  case MSG_ID_NET_IP_CHANGE:
    lwip_reset_netif_ipaddr();
//    if(MsgDeal_Send(get_net_service_handler(), get_tcp_service_handler(), MSG_ID_TCP_CLOSE, NULL, 0) != 0)
//    {
//      logerr("net send to tcp service fail !\r\n");
//    }
    loginf("ip change\r\n");
    break;
  case MSG_ID_NET_REMOTE_IP_CHANGE:
    if(MsgDeal_Send(get_net_service_handler(), get_tcp_service_handler(), MSG_ID_TCP_CLOSE, NULL, 0) != 0)
    {
      logerr("net send to tcp service fail !\r\n");
    }
    loginf("remote ip change\r\n");
    break;  
  default:
    logerr("net service ID err\r\n");
    break;
  }
}



/* 网络服务包含udp和tcp服务 */
void start_net_service(void)
{
  /* 初始化硬件和网络协议栈 */
  lwip_init_l();
  /* 网络 服务 */
  OS_ERR  os_err;
  
  OSMutexCreate(&NETlock, "NETlock", &os_err);
  assert(os_err == OS_ERR_NONE);
  
  OSQCreate (&net_service_q,"net_service_q",10,&os_err);
  assert (os_err == OS_ERR_NONE);
  OSTaskCreate((OS_TCB     *)&net_service_tcb,                               /* Create the startup task                              */
               (CPU_CHAR   *)"net service",
               (OS_TASK_PTR )net_service,
               (void       *)0u,
               (OS_PRIO     )APP_CFG_NET_TASK_PRIO,
               (CPU_STK    *)&net_service_stk[0u],
               (CPU_STK_SIZE)APP_CFG_NET_TASK_STK_SIZE / 10u,
               (CPU_STK_SIZE)APP_CFG_NET_TASK_STK_SIZE,
               (OS_MSG_QTY  )0u,
               (OS_TICK     )0u,
               (void       *)0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR     *)&os_err);
  assert (os_err == OS_ERR_NONE) ;
  

}

/* net 服务 */
static  void  net_service (void *p_arg)
{
  MSG_BUF_t* recv_info  = NULL;
  lwip_dev_t dev = {0};
  get_lwip_dev(&dev);
  loginf("Local ip = %d.%d.%d.%d\r\n" ,dev.ip[0], dev.ip[1], dev.ip[2], dev.ip[3]);
  loginf("Remote ip = %d.%d.%d.%d\r\n" ,dev.remoteip[0], dev.remoteip[1], dev.remoteip[2], dev.remoteip[3]);
  while (1)
  {
    if(MsgDeal_Recv(get_net_service_handler(), &recv_info, 5000) == 0)
    {
      handle_msg(recv_info);
      MsgDeal_Free(recv_info);
    }
    set_task_alive_state(NET_SERVICE_TASK);
  }
}
