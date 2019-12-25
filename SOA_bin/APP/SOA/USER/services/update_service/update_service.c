#include  "stm32f1xx_hal.h"
#include  "app_cfg.h"
#include  "lwip.h"
#include  "lwip/api.h"
#include  <assert.h>
#include  "hl_msgdeal.h"
#include  "sFlash.h"
#include  "crc.h"
#include  "data_service.h"
#include  "net_service.h"
#include  "tcp_service.h"
#include  "update_service.h"
#include  "register_service.h"
#include  "string.h"
#include  "log.h"
#include  "delay.h"
/* update 服务所需要的资源 */
static  OS_Q  		update_service_q;
static  OS_Q  		update_data_q;
static  void  		update_service (void  *p_arg);
static  OS_TCB   	update_service_tcb;
static  CPU_STK  	update_service_stk[APP_CFG_UPDATE_TASK_STK_SIZE];


OS_Q*  get_update_service_handler(void)
{
  return &update_service_q;
}

OS_Q* get_update_data_handler(void)
{
  return &update_data_q;
}

__packed typedef struct
{
  uint8_t	headflg; 		//与通用报文一致
  uint16_t	seq;      		//升级序列号，同一个文件升级时，seq必须一样
  uint16_t	tpage;  		//总的包数
  uint16_t	cpage; 			//当前包数
  uint16_t   	plen;   		//payload的长度
  uint8_t	payload[256];  	        //payload
  uint8_t	crc8;   		//crc校验，与通用报文一致
  uint8_t	endflg; 		//与通用报文一致	
}Pkt_t;


typedef enum
{
  IDLE,
  WAIT_HEAD,
  WAIT_SEQ,
  WAIT_TOTAL,
  WAIT_CUR,
  WAIT_LEN,
  WAIT_PAYLOAD,
  WAIT_CRC,
  WAIT_END
}UpdateStat_t;

typedef struct
{
  uint8_t 	pkt_flag ;
  uint8_t 	pkt_errtimes ;
  uint16_t 	cpkt	;
  uint16_t 	need_len ;
  uint16_t 	len_index ;
}UpdateIndex_t;


//static UpdateStat_t 	upstat = IDLE;
static Pkt_t		pkt = {0};
static UpdateIndex_t 	upindex = {0};	//升级进度
uint32_t dataaddr = 0;

static uint16_t			pack_sn = 0;
static uint8_t                  firstPktFlag = RESET;

/* 升级服务 */
void start_update_service(void)
{
  OS_ERR  os_err;
  
  OSQCreate (&update_service_q,"update_service_q",10,&os_err);
  assert (os_err == OS_ERR_NONE);
  
  OSQCreate (&update_data_q,"update_data_q",10,&os_err);
  assert (os_err == OS_ERR_NONE) ;
  
  OSTaskCreate((OS_TCB     *)&update_service_tcb,                               /* Create the startup task                              */
               (CPU_CHAR   *)"update service",
               (OS_TASK_PTR )update_service,
               (void       *)0u,
               (OS_PRIO     )APP_CFG_UPDATE_TASK_PRIO,
               (CPU_STK    *)&update_service_stk[0u],
               (CPU_STK_SIZE)APP_CFG_UPDATE_TASK_STK_SIZE / 10u,
               (CPU_STK_SIZE)APP_CFG_UPDATE_TASK_STK_SIZE,
               (OS_MSG_QTY  )0u,
               (OS_TICK     )0u,
               (void       *)0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR     *)&os_err);
  assert (os_err == OS_ERR_NONE) ;
  

}

static uint8_t _app_info_is_ok(uint8_t* addr)
{
  uint8_t r_buf[64];
  uint8_t crc8;
  memcpy(r_buf ,addr , 64);
  crc8 = CRC8_CCITT_Calc(r_buf, 63);
  if(crc8 == r_buf[63])
  {
    loginf("crc %d\r\n", crc8);
    return 1;
  }
  else 
  {
    logerr("crc err %d\r\n", crc8);
    return 0;
  }
}

/*  
errno 错误号
data  需要时为包的序号，不需要时该参数为0
sn    消息流水号，该参数从0开始自增
*/
static int send_ack_to_platform(uint8_t errno , uint16_t data , uint16_t* psn)
{
  uint16_t sn = *psn;
  (*psn)++ ;
  uint8_t pbuf[128] = {0};
  pbuf[0] = 0x68;
  get_addr((char*)pbuf + 1);
  pbuf[14] = sn & 0xFF;
  pbuf[15] = (sn >> 8) & 0xFF;
  pbuf[16] = 0xF1;
  pbuf[17] = 0x04;
  pbuf[18] = 0x00;
  
  pbuf[19] = 0x15;
  pbuf[20] = errno;
  pbuf[21] = data & 0xFF;
  pbuf[22] = (data >> 8) & 0xFF;
  
  uint16_t crc = crc16_modbus(pbuf + 1, 22);
  pbuf[23] = crc & 0xFF;
  pbuf[24] = (crc >> 8) & 0xFF;	
  
  pbuf[25] = 0x16;
  if(MsgDeal_Send(get_update_service_handler(), get_tcp_service_handler(), MSG_ID_TCP_PACK, pbuf, 26) != 0)
  {
    printf("update service send to tcp service ack fail !\r\n");
    return -1;
  }
  return 0;
}

static uint8_t pkt_crc_is_ok(Pkt_t* mpkt)
{
  uint32_t dlen = mpkt->plen;
  
  uint8_t crc8 = 0;
  crc8 = mpkt->payload[dlen];
  uint8_t crc = 0;
  crc = CRC8_CCITT_Calc((uint8_t*)mpkt + 1, dlen + 8);
  if(crc8 != crc)
    return 0;
  else
    return 1;
}

int upgrade_data_handler(MSG_BUF_t* msg)
{
    //错误数大于3
    if(upindex.pkt_errtimes >3)
    {
        return 1;
    }  
    if(msg->len > sizeof(Pkt_t))
    {
      //一包最大256字节
      send_ack_to_platform(0x14, 0x00, &pack_sn);
      upindex.pkt_errtimes++;
      logerr("upgrade len err\r\n");
      return 0;
    }
    memcpy(&pkt, msg->buf, msg->len);
    if(!pkt_crc_is_ok(&pkt))
    {
      send_ack_to_platform(0x14, pkt.cpage, &pack_sn);
      upindex.pkt_errtimes++;
      return 0;
    }
    if(firstPktFlag == SET)
    {
      Img_head_t Img_head = {0};
      memcpy(&Img_head, pkt.payload, sizeof(Img_head_t));    
      if(Img_head.devType != 0x0102)
      {
        send_ack_to_platform(0x10, 0x01, &pack_sn);
        logerr("upgrade DEV_TYPE err\r\n");
        return 1;
      }
      if(Img_head.softVersion != get_upgrade_software_version())
      {
        send_ack_to_platform(0x10, 0x02, &pack_sn);
        logerr("upgrade softVersio err\r\n");
        return 1;
      }
      if(!_app_info_is_ok(pkt.payload))
      {
        send_ack_to_platform(0x10, 0x03, &pack_sn);
        logerr("upgrade softVersio err\r\n");
        return 1;
      }
      loginf("total page %d\r\n", pkt.tpage);
      firstPktFlag = RESET;
    }
    //包序号错误
    if(pkt.cpage != upindex.cpkt+1)
    {
      send_ack_to_platform(0x12, upindex.cpkt+1, &pack_sn);
      //upindex.pkt_errtimes++;
      return 0;
    }
    //写flash
    sFlash_WriteBytes(dataaddr, pkt.payload, pkt.plen);
    upindex.cpkt = pkt.cpage;
    upindex.pkt_errtimes = 0;
    if(pkt.cpage != pkt.tpage)
    {
      dataaddr += pkt.plen;
      loginf("update Data ok %d\r\n", pkt.cpage);
    }
    else
    {
      send_ack_to_platform(0x10, 0x00, &pack_sn);
      upindex.pkt_errtimes = 0;
      loginf("update finish\r\n")
      return 2;
    }
    return 0;
}

int handle_upgrade_pkt(MSG_BUF_t* msg)
{
  int ret = 0;
  switch(msg->mtype)
  {
    case MSG_ID_TCP_PACK:
        ret = upgrade_data_handler(msg);
        break;
    default:
        logerr("upgrade data id error\r\n");
        break;
  }
  return ret;
}

void upgrade_handler()
{
    int ret = 0;
    MSG_BUF_t* msg  = NULL;
    
    firstPktFlag = SET;
    dataaddr = 0;
    memset(&upindex, 0, sizeof(UpdateIndex_t));
    //delay_ms(200);
    send_ack_to_platform(0x11, 0, &pack_sn);
    
    while (1)
    {
      if(MsgDeal_Recv(get_update_data_handler(), &msg, 5000) == 0)
      {
        ret = handle_upgrade_pkt(msg);
        MsgDeal_Free(msg);
      }
      else
      {
        upindex.pkt_errtimes++;
        if(upindex.pkt_errtimes >0)
        {
          if(ret != 2)
          {
            send_ack_to_platform(0x13, upindex.cpkt+1, &pack_sn);
            logerr("upgrade timeout err\r\n");
          }
        }
        if(upindex.pkt_errtimes >3)
        {
          send_ack_to_platform(0x10, 0x06, &pack_sn);
          set_device_state(DEVICE_STATE_WORK);
          logerr("upgrade timeout errtimes 3\r\n");
          break;
        }
        if(ret != 0)
        {
          if(ret == 2)
          {
            set_config_upgradeFlag(NEED_UPDAGRADE);
            delay_ms(200);
            loginf("update state: %d !\r\n", sysconfig.upgradeState);
            uint8_t log_cause;
            log_cause = OUT_CAUSE_UPGRADE;
            if(MsgDeal_Send(get_update_service_handler(), get_register_service_handler(), MSG_ID_LOG_OUT, &log_cause, 1) != 0)
            {
              logerr("update service pack send to register service fail !\r\n");
            }
            break;
          }
          set_device_state(DEVICE_STATE_WORK);
          logerr("upgrade timeout errtimes 3\r\n");
          break;
        }
      }
    }
}

void report_upgradeFinish(void)
{
  SysConf_t config = {0};
  get_config(&config); 
  if(config.upgradeState == FINISH_UPDAGRADE)
  {
    send_ack_to_platform(0x20, 0x00, &pack_sn);
    loginf("FINISH_UPDAGRADE\r\n");
  }
  else if(config.upgradeState == SELF_TEST_FAIL)
  {
    send_ack_to_platform(0x01, 0x00, &pack_sn);
  }
  set_config_upgradeFlag(NO_UPDAGRADE);
  loginf("SET NO_UPDAGRADE\r\n");
}

void handle_msg(MSG_BUF_t* msg)
{
  switch(msg->mtype)
  {
    case MSG_ID_UPGRADE_IND:
        upgrade_handler();
        break;
    case MSG_ID_LOG_IN:
        report_upgradeFinish();
        break;
    default:
        logerr("upgrade id error\r\n");
        break;
  }
}

/* update 服务 */
static  void  update_service (void *p_arg)
{
  (void)p_arg;
  MSG_BUF_t* recv_info  = NULL;
  
  while(DEF_TRUE)
  {
    if(MsgDeal_Recv(get_update_service_handler(),&recv_info, 5000) == 0)
    {
      handle_msg(recv_info);
      MsgDeal_Free(recv_info);
    }
  }
}