#include  "stm32f1xx_hal.h"
#include  "app_cfg.h"
#include  "lwip.h"
#include  "lwip/api.h"
#include  <assert.h>
#include  "hl_msgdeal.h"
#include  "hl_packet.h"
#include  "data_service.h"
#include  "update_service.h"
#include  "register_service.h"
#include  "tcp_service.h"
#include  "CRC.h"
#include  "string.h"
#include  "net_service.h"
#include  "supervision_service.h"
#include  "log.h"
#include  "ip.h"
#include  "tcp.h"

/* tcp 服务所需要的资源 */
static  OS_Q  		tcp_service_q;
static  void  		tcp_service (void  *p_arg);
static  OS_TCB   	tcp_service_tcb;
static  CPU_STK  	tcp_service_stk[APP_CFG_TCP_TASK_STK_SIZE];

MSG_BUF_t* tcp_msg = NULL;
struct netconn *tcpconn = NULL;

struct netbuf  *recvbuf;
uint16_t netbuf_len = 0;
static uint8_t packed_data[1024] = {0};
lwip_dev_t dev = {0};

OS_Q*  get_tcp_service_handler(void)
{
  return &tcp_service_q;
}

OS_TCB get_tcp_service_tcb(void)
{
  return tcp_service_tcb;
}

void close_tcp_conn(void)
{
  if(tcpconn != NULL)
  {
    netconn_close(tcpconn);
    netconn_delete(tcpconn);
    tcpconn = NULL;
  }
  loginf("disconnect !\r\n");
  set_device_state(DEVICE_STATE_NOT_RDY);
}

void keep_alive_pram_change(void)
{
  SysConf_t config = {0};
  get_config(&config);
  if(tcpconn != NULL)
  {
    tcpconn->pcb.tcp->keep_idle = (3*config.report_interval + 5)*1000;
    loginf("keep alive: %d\r\n", tcpconn->pcb.tcp->keep_idle);
  }
}


static void handle_msg(MSG_BUF_t* msg)
{
  loginf("tcp service receive data! datalen = %d\r\n" , msg->len);
  switch(msg->mtype)
  {
  case MSG_ID_TCP_PACK:
    //loginf("test1\r\n");
    if(netconn_write(tcpconn ,(char*)(msg->buf),msg->len,NETCONN_COPY) != ERR_OK)
    {
      loginf("tcp service send pack to server fail!\r\n");
      close_tcp_conn();
    }
    //loginf("test2\r\n");
    break;
  case MSG_ID_UPGRADE_RESET:
    close_tcp_conn();
    __disable_irq();
    HAL_NVIC_SystemReset();
    break;
  case MSG_ID_TCP_CLOSE:
    close_tcp_conn();
    break;
  case MSG_ID_HEARTBEAT_DURATION_CHANGE:
    keep_alive_pram_change();
    break;  
  default:
    loginf("tcp service ID err\r\n");
    break;
  }
}

static uint8_t connect_remote_host(void)
{
  err_t lwip_err;
  SysConf_t config = {0};
  get_config(&config); 
  get_lwip_dev(&dev);
  
  ip_addr_t  	tcp_remote_ip = {0};	
  IP4_ADDR(&tcp_remote_ip, dev.remoteip[0],dev.remoteip[1], dev.remoteip[2],dev.remoteip[3]);
  tcpconn = netconn_new(NETCONN_TCP);
  assert(tcpconn != NULL);
  if(tcpconn != NULL)
  {
    //tcpconn->flags |= NETCONN_FLAG_NON_BLOCKING;
    tcpconn->recv_timeout = 100;  //从消息邮箱接受消息等待的时间  
    tcpconn->send_timeout = 500;

    tcpconn->pcb.tcp->keep_idle = (3*config.report_interval + 5)*1000;
    tcpconn->pcb.tcp->keep_intvl = 5;
    tcpconn->pcb.tcp->keep_cnt = 3;
    ip_set_option(tcpconn->pcb.tcp, SOF_KEEPALIVE);
  }			
  lwip_err = netconn_connect(tcpconn,&tcp_remote_ip,dev.remoteport);	
  if(lwip_err != ERR_OK)
  {
    netconn_delete(tcpconn);
    return ERROR;
  }
  else
  {
    return SUCCESS;
  }
}

uint8_t log_flag = RESET;

static void notify_register(void)
{
  loginf("Connect Remote Success ! ip = %d.%d.%d.%d , port = %d\r\n" ,dev.remoteip[0],dev.remoteip[1], dev.remoteip[2],dev.remoteip[3], dev.remoteport);
  /*连接成功*/
  set_device_state(DEVICE_STATE_STAND_BY);
  uint8_t log_cause;
  if(log_flag == RESET)
  {
    log_flag = SET;
    log_cause = CAUSE_SYS_RESET;
  }
  else
  {
    log_cause = CAUSE_RECONNECT;
  }
  if(MsgDeal_Send(get_tcp_service_handler(), get_register_service_handler(), MSG_ID_LOG_IN, &log_cause, 1) != 0)
  {
    loginf("tcp pack send to register service fail !\r\n");
  }
  if(MsgDeal_Send(get_tcp_service_handler(), get_update_service_handler(), MSG_ID_LOG_IN, NULL, 0) != 0)
  {
    loginf("tcp pack send to update service fail !\r\n");
  }
}

static int _SearchFlag(char* buf , int len ,char flag)
{
  for(int i = 0 ; i < len ; i++)
  {
    if(buf[i] == flag )
      return i;
  }
  return -1;
}

static int _GetTotalLen(char* buf)
{
  uint8_t headlen;
  headlen = sizeof(PKT_HEAD_S);
  int data_len = (int)(buf[headlen-2] | buf[headlen-1] << 8) ;
  //包头+ 包尾 + 校验
  return (data_len + headlen + 2 + 1);
}

static int _GetUpgradeTotalLen(char* buf)
{
  int data_len = (int)(buf[7] | buf[8] << 8) ;
  //包头+ 包尾 + 校验
  return (data_len + 11);
}


static int _CheckPackValid(char* buf, int len)
{
  int ret = 0;
  uint16_t crc = 0;
  crc = crc16_modbus((unsigned char *)buf + 1,len - 4);
  uint16_t crc_pack = buf[len - 2] << 8 | buf[len - 3];
  if(crc_pack != crc)
    ret = -1;
  return ret;
}

static int _CheckUpgradePackValid(char* buf, int len)
{
  int ret = 0;
  uint8_t crc = 0;
  crc = CRC8_CCITT_Calc((unsigned char *)buf + 1,len - 3);
  uint8_t crc_pack = buf[len - 2];
  if(crc_pack != crc)
    ret = -1;
  return ret;
}


/*******************************************************************************
* Function Name  : parse_stream
* Description    : 流处理函数，
* Input          : 缓冲区中 头数据指针  数据长度   
* Output         : 正常要发送的包的数据长度, 收到的数据中由于异常丢弃的数据长度
* Return         : 是不是一个完整的包
*******************************************************************************/
int parse_data_stream(void  *buf, uint16_t len, char **pstart, uint16_t* senddata_len, int16_t* errordata_len)
{
  char* pbuf = (char*)buf;
  int offset = _SearchFlag(pbuf,len ,0x68);
  int TotalLen = 0;
  //没有找到0x68, 数据全部丢弃
  if(offset<0)
  {
    *errordata_len = -1;
    return ERROR;
  }
  pbuf += offset;
  //包长度还不够到长度的判断字段的地方
  if(offset + sizeof(PKT_HEAD_S) - 2 > len)
  {
    *errordata_len = offset;
    return ERROR;
  }
  TotalLen = _GetTotalLen(pbuf);
  //长度字段异常  本项目指令长度不会超过300
  if(TotalLen > 300)
  {
    *errordata_len = offset + 1;
    return ERROR;
  }
  //包总长度不够
  if((offset + TotalLen) > len)
  {
    //把68前面的数据丢弃
    *errordata_len = offset;
    return ERROR;
  }
  //结束符不对
  if(pbuf[TotalLen-1] != 0x16)
  {
    *errordata_len = offset + 1;
    return ERROR;
  }
  int ret = 0;
  ret = _CheckPackValid(pbuf, TotalLen);//校验失败
  if(ret < 0)
  {
    *errordata_len = offset + 1;
    logerr("crc_error\r\n");
    return ERROR;
  }
  *pstart = pbuf;
  *senddata_len = TotalLen;
  *errordata_len = offset + TotalLen;
  return SUCCESS;
}

/*******************************************************************************
* Function Name  : parse_stream
* Description    : 流处理函数，
* Input          : 缓冲区中 头数据指针  数据长度   
* Output         : 正常要发送的包的数据长度, 收到的数据中由于异常丢弃的数据长度
* Return         : 是不是一个完整的包
*******************************************************************************/
int parse_upgrade_stream(void  *buf, uint16_t len, char **pstart, uint16_t* senddata_len, int16_t* errordata_len)
{
  char* pbuf = (char*)buf;
  int offset = _SearchFlag(pbuf,len ,0x68);
  int TotalLen = 0;
  //没有找到0x68, 数据全部丢弃
  if(offset<0)
  {
    *errordata_len = -1;
    return ERROR;
  }
  pbuf += offset;
  //包长度还不够到长度的判断字段的地方
  if(offset + 5 > len)
  {
    *errordata_len = offset;
    return ERROR;
  }
  TotalLen = _GetUpgradeTotalLen(pbuf);
  //长度字段异常  本项目指令长度不会超过300
  if(TotalLen > 267)
  {
    *errordata_len = offset + 1;
    return ERROR;
  }
  //包总长度不够
  if((offset + TotalLen) > len)
  {
    //把68前面的数据丢弃
    *errordata_len = offset;
    return ERROR;
  }
  //结束符不对
  if(pbuf[TotalLen-1] != 0x16)
  {
    *errordata_len = offset + 1;
    return ERROR;
  }
  int ret = 0;
  ret = _CheckUpgradePackValid(pbuf, TotalLen);//校验失败
  if(ret < 0)
  {
    *errordata_len = offset + 1;
    logerr("crc_error\r\n");
    return ERROR;
  }
  *pstart = pbuf;
  *senddata_len = TotalLen;
  *errordata_len = offset + TotalLen;
  return SUCCESS;
}

static void tcp_stream_handler()
{
  int16_t errorData_len = 0;
  uint16_t packet_len = 0;
  struct pbuf *q;
  char* dataptr = recvbuf->p->payload;
  uint8_t pack_state = ERROR; 
  for(q=recvbuf->p; q!=NULL; q=q->next)
  {
    loginf("receive server data , data length = %d\r\n" , q->len);
    if(netbuf_len + q->len > 1024)
    {
      netbuf_len = 0;
      netbuf_delete(recvbuf);
      logerr("netbuf_len error\r\n");
      return;
    }
    memcpy(packed_data + netbuf_len, q->payload, q->len);
    netbuf_len += q->len;
    while(netbuf_len)
    {
      if(get_device_state() == DEVICE_STATE_UPGRADE)
      {
        pack_state = parse_upgrade_stream(packed_data, netbuf_len, &dataptr, &packet_len, &errorData_len);
      }
      else
      {
        pack_state = parse_data_stream(packed_data, netbuf_len, &dataptr, &packet_len, &errorData_len);
      }
      if(pack_state == SUCCESS)
      {
        loginf("pack_state SUCCESS %d\r\n", packet_len);
        /* 获得 数据的地址和长度后 ,将数据传输至 其他服务解析及控制 */
        if(get_device_state() == DEVICE_STATE_UPGRADE)
        {
          int rtn;
          rtn = MsgDeal_Send(get_tcp_service_handler(), get_update_data_handler(), MSG_ID_TCP_PACK, dataptr, packet_len);
          loginf("rtn %d\r\n", rtn);
          if(rtn != 0)
          {
            loginf("tcp pack send to update service fail !\r\n");
          }
        }
        else
        {
          if(MsgDeal_Send(get_tcp_service_handler(), get_data_service_handler(), MSG_ID_TCP_PACK, dataptr, packet_len) != 0)
          {
            loginf("tcp pack send to data service fail !\r\n");       
          }
        }
      }
      else
      {
        if(errorData_len <0 )
        {
          netbuf_len = 0;
          loginf("Not Find 0x68\r\n");
          break;
        }
        else if(errorData_len == 0)
        {
          break;
        }
        loginf("error data\r\n");
      }
      netbuf_len -= errorData_len;
      //拷贝pbuf 剩下的数据
      memcpy(packed_data, packed_data + errorData_len, netbuf_len);
    }
  }
  netbuf_delete(recvbuf);
}
/* tcp服务 */
void start_tcp_service(void)
{
  /* tcp 服务 */
  OS_ERR  os_err;
  OSQCreate (&tcp_service_q,"tcp_service_q",10,&os_err);
  assert (os_err == OS_ERR_NONE);
  
  OSTaskCreate((OS_TCB     *)&tcp_service_tcb,                               /* Create the startup task                              */
               (CPU_CHAR   *)"tcp service",
               (OS_TASK_PTR )tcp_service,
               (void       *)0u,
               (OS_PRIO     )APP_CFG_TCP_TASK_PRIO,
               (CPU_STK    *)&tcp_service_stk[0u],
               (CPU_STK_SIZE)APP_CFG_TCP_TASK_STK_SIZE / 10u,
               (CPU_STK_SIZE)APP_CFG_TCP_TASK_STK_SIZE,
               (OS_MSG_QTY  )0u,
               (OS_TICK     )0u,
               (void       *)0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR     *)&os_err);
  assert (os_err == OS_ERR_NONE) ;
}

/* tcp 服务 */
static  void  tcp_service (void *p_arg)
{
  OS_ERR  err;
  err_t lwip_err;
  MSG_BUF_t* recv_info  = NULL;
  while(1)
  {
    if(tcpconn == NULL)
    {
      //if(get_logout_flag() == RESET)
      {
        if(connect_remote_host() == ERROR)
        {
          set_device_state(DEVICE_STATE_NOT_RDY);
          tcpconn = NULL;
          loginf("No connect \r\n");
          OSTimeDly (1000,OS_OPT_TIME_DLY,&err);
          set_task_alive_state(TCP_SERVICE_TASK);
          continue;
        }
        else
        {
          notify_register();
        }
      }
    }
    else
    {
      lwip_err = netconn_recv(tcpconn,&recvbuf);
      if(lwip_err == ERR_OK)
      {
        tcp_stream_handler();
      }
      else if(lwip_err != ERR_TIMEOUT)
      {
        logerr("lwip_err %d\r\n", lwip_err);
        close_tcp_conn();
      }
      if(MsgDeal_Recv(get_tcp_service_handler(), &recv_info, 10) == 0)
      {
        handle_msg(recv_info);
        MsgDeal_Free(recv_info);
      }    
      set_task_alive_state(TCP_SERVICE_TASK);
    }
  }	
}


