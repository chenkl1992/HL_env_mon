#include  "stm32f1xx_hal.h"
#include  "app_cfg.h"
#include  "lwip.h"
#include  "lwip/api.h"
#include  <assert.h>
#include  "hl_msgdeal.h"
#include  "data_service.h"
#include  "update_service.h"
#include  "supervision_service.h"
#include  "log.h"

#define UDP_PORT                40006

/* udp 服务所需要的资源 */
static  OS_Q  		udp_service_q;
static  void  		udp_service (void  *p_arg);
static  OS_TCB   	udp_service_tcb;
static  CPU_STK  	udp_service_stk[APP_CFG_UDP_TASK_STK_SIZE];

volatile static uint8_t connected_flag = 0;
struct netconn *udpconn = NULL;

OS_Q*  get_udp_service_handler(void)
{
  return &udp_service_q;
}

OS_TCB get_udp_service_tcb(void)
{
  return udp_service_tcb;
}

/* udp服务 */
void start_udp_service(void)
{
  OS_ERR  os_err;
  OSQCreate (&udp_service_q,"udp_service_q",10,&os_err);
  assert (os_err == OS_ERR_NONE);
  
  /* udp 服务 */
  OSTaskCreate((OS_TCB     *)&udp_service_tcb,                               /* Create the startup task                              */
               (CPU_CHAR   *)"udp service",
               (OS_TASK_PTR )udp_service,
               (void       *)0u,
               (OS_PRIO     )APP_CFG_UDP_TASK_PRIO,
               (CPU_STK    *)&udp_service_stk[0u],
               (CPU_STK_SIZE)APP_CFG_UDP_TASK_STK_SIZE / 10u,
               (CPU_STK_SIZE)APP_CFG_UDP_TASK_STK_SIZE,
               (OS_MSG_QTY  )0u,
               (OS_TICK     )0u,
               (void       *)0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR     *)&os_err);
  assert (os_err == OS_ERR_NONE) ;	
}

void udp_data_send(MSG_BUF_t* recv_info)
{
  struct netbuf  *sentbuf;
  char* dataptr = NULL;
  
  loginf("udp service receive data service data! datalen = %d\r\n" , recv_info->len);
  sentbuf = netbuf_new();
  dataptr = netbuf_alloc(sentbuf,recv_info->len);
  Mem_Copy(dataptr, (char*)(recv_info->buf), recv_info->len);
  if(netconn_send(udpconn,sentbuf) != ERR_OK)
  {
    logerr("udp server send pack to server fail!\r\n");
  }				
  netbuf_delete(sentbuf);
}

void udp_recive(void)
{
  u16_t dlen = 0;
  struct netbuf  *recvbuf;
  ip_addr_t  	udp_remote_ip = {0}; 
  u16_t		udp_remote_port = 0; 
  
  netconn_recv(udpconn,&recvbuf);
  if(recvbuf != NULL)  //接收到数据
  {
    char* dataptr = NULL;
    udp_remote_ip = recvbuf->addr;
    udp_remote_port = recvbuf->port;
    netconn_connect(udpconn,&udp_remote_ip,udp_remote_port);	
    loginf("ip = %d , port = %d\r\n" , udp_remote_ip.addr , udp_remote_port);
    netbuf_data(recvbuf , (void*)&dataptr , &dlen);
    if(MsgDeal_Send(get_udp_service_handler(), get_data_service_handler(), MSG_ID_UDP_PACK, dataptr, dlen) != 0)
    {
      logerr("udp service pack send to data service fail !\r\n");
    }
    netbuf_delete(recvbuf);
  }
}

void udp_service_init(void)
{
  lwip_dev_t dev = {0};
  get_lwip_dev(&dev);
  
  ip_addr_t ip_addr = {0};
  IP4_ADDR(&ip_addr,dev.ip[0],dev.ip[1], dev.ip[2],dev.ip[3]);
  
  udpconn = netconn_new(NETCONN_UDP);
  assert(udpconn != NULL);
  if(udpconn != NULL)
  {
    udpconn->recv_timeout = 100;  //从消息邮箱接受消息等待的时间
    netconn_bind(udpconn,&ip_addr, UDP_PORT);
  }
}
static void handle_msg(MSG_BUF_t* msg)
{
  switch(msg->mtype)
  {
  case MSG_ID_UDP_PACK:
    udp_data_send(msg);
    break;
  default:
    logerr("udp ID err\r\n");
    break;
  }
}

/* udp 服务 */
static  void  udp_service (void *p_arg)
{
  (void)p_arg;

  MSG_BUF_t* recv_info  = NULL;
  udp_service_init();
  while(1)
  {
    if(udpconn != NULL)
    {
      udp_recive();
    }
    if(MsgDeal_Recv(get_udp_service_handler(), &recv_info, 10) == 0)
    {
      handle_msg(recv_info);
      MsgDeal_Free(recv_info);
    }
    set_task_alive_state(UDP_SERVICE_TASK);
  }
}