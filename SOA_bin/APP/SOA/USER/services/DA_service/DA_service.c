#include  "stm32f1xx_hal.h"
#include  "app_cfg.h"
#include  "lwip.h"
#include  "lwip/api.h"
#include  <assert.h>
#include  <math.h>
#include  "hl_msgdeal.h"
#include  "data_service.h"
#include  "tcp_service.h"
#include  "DA_service.h"
#include  "net_service.h"
#include  "event_service.h"
#include  "delay.h"
#include  "CRC.h"
#include  "string.h"
#include  "sds.h"
#include  "supervision_service.h"
#include  "log.h"
#include  "stdbool.h"


/* 数据采集 服务所需要的资源 */
static  OS_Q  		DA_service_q;
static  void  		DA_service (void  *p_arg);
static  OS_TCB   	DA_service_tcb;
static  CPU_STK  	DA_service_stk[APP_CFG_DA_TASK_STK_SIZE];
OS_MUTEX DAlock;
OS_ERR  DA_err;

static  OS_Q  		DA_service_ack_q;


#define DATALEN_MAX                     100

#define SENSOR_PROSESS                  1
#define SENSOR_GET                      0

#define FUNC_CODE_READ_485              3


Sensor_State_t Sensor_State[DEV_NUM] = {0};
Monitor_Data Monitor = {0};

uint8_t DAbuf[DATALEN_MAX];
uint8_t cmd[8] = {0};
uint16_t data_len = 0;
uint8_t DA_Init_flag = RESET;
static uint8_t DA_Init_counter = 0;

OS_TMR DA_tmr;

typedef struct {
  uint8_t type;
  void (*fun)(uint8_t, uint8_t);
}R_Data;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
DMA_HandleTypeDef hdma_usart2_tx;


OS_Q*  get_DA_service_handler(void)
{
  return &DA_service_q;
}

OS_TCB get_DA_service_tcb(void)
{
  return DA_service_tcb;
}

OS_Q*  get_DA_service_ack_handler(void)
{
  return &DA_service_ack_q;
}

void DA_uart_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();
  
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 19200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    assert(0);
  }
  
  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  
  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);
  
  /**USART2 GPIO Configuration    
  PA2     ------> USART2_TX
  PA3     ------> USART2_RX 
  */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  /* USART2 DMA Init */
  /* USART2_RX Init */
  hdma_usart2_rx.Instance = DMA1_Channel6;
  hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_usart2_rx.Init.Mode = DMA_NORMAL;
  hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
  
  if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
  {
    assert(0);
  }
  
  __HAL_LINKDMA(&huart2,hdmarx,hdma_usart2_rx);
  /* USART2_TX Init */
  hdma_usart2_tx.Instance = DMA1_Channel7;
  hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
  hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_usart2_tx.Init.Mode = DMA_NORMAL;
  hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
  if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
  {
    assert(0);
  }
  __HAL_LINKDMA(&huart2,hdmatx,hdma_usart2_tx);
  /* USART2 interrupt Init */
  HAL_NVIC_SetPriority(USART2_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(USART2_IRQn);
  
}

void get_Monitor(Monitor_Data* Monitor_data)
{
  OSMutexPend(&DAlock, 0, OS_OPT_PEND_BLOCKING, NULL, &DA_err);
  assert(DA_err == OS_ERR_NONE);
  *Monitor_data = Monitor;
  OSMutexPost(&DAlock, OS_OPT_POST_NONE, &DA_err);
  assert(DA_err == OS_ERR_NONE);
}

void get_sensor_State(Sensor_State_t* State, uint8_t type)
{
  OSMutexPend(&DAlock, 0, OS_OPT_PEND_BLOCKING, NULL, &DA_err);
  assert(DA_err == OS_ERR_NONE);
  memcpy(State, &(Sensor_State[type]) , sizeof(Sensor_State_t));
  OSMutexPost(&DAlock, OS_OPT_POST_NONE, &DA_err);
  assert(DA_err == OS_ERR_NONE);
}

void set_sensor_State(Sensor_State_t* State, uint8_t type)
{
  OSMutexPend(&DAlock, 0, OS_OPT_PEND_BLOCKING, NULL, &DA_err);
  assert(DA_err == OS_ERR_NONE);
  memcpy(&(Sensor_State[type]), State, sizeof(Sensor_State_t));
  OSMutexPost(&DAlock, OS_OPT_POST_NONE, &DA_err);
  assert(DA_err == OS_ERR_NONE);
}

void set_sys_sensor_State(Sensor_State_t* State, uint8_t type)
{
  OSMutexPend(&DAlock, 0, OS_OPT_PEND_BLOCKING, NULL, &DA_err);
  assert(DA_err == OS_ERR_NONE);
  sysconfig_edit();
  memcpy(&(sysconfig.Sensor_State[type]), State, sizeof(Sensor_State_t));
  sysconfig.Sensor_State[type].errorFlag = RESET;
  sysconfig_save();
  OSMutexPost(&DAlock, OS_OPT_POST_NONE, &DA_err);
  assert(DA_err == OS_ERR_NONE);
  SDS_Poll();
}

unsigned long  BCDtoDec( unsigned char *bcd, int length) 
{ 
     int i, tmp; 
     unsigned long dec = 0;
     for(i=0; i<length; i++) 
     { 
        tmp = ((bcd[i]>>4)&0x0F)*10 + (bcd[i]&0x0F);    
        dec += tmp * pow(100, length-1-i);           
     }
     return dec; 
}	

/*******************************************************
addr 485地址
reg_addr 寄存器地址
reg_num 寄存器个数

*******************************************************/
void send_modbus_cmd(uint8_t addr,uint16_t reg_addr, uint16_t reg_num)
{
  uint16_t crc;
  cmd[0] = addr;
  cmd[1] = FUNC_CODE_READ_485;
  cmd[2] = reg_addr >> 8;
  cmd[3] = reg_addr & 0xff;
  cmd[4] = (reg_num >>8) & 0xff;
  cmd[5] = reg_num & 0xff;
  crc = crc16_modbus(cmd,6);
  cmd[6] =  crc & 0xff;
  cmd[7] = crc >> 8;
  memset(DAbuf,0,DATALEN_MAX);
  data_len = 0;
  HAL_UART_Transmit_DMA(&huart2, cmd, 8);
}


uint8_t get_modbus_reply(uint8_t* data, uint8_t data_len, uint8_t type, uint8_t addr)
{
  uint8_t parm_len;
  uint16_t crc;
  //判断数据长度
  if(type == 0)
  {    
    if(data_len != 43)
    {
      return 1;
    }
  }
  if(type == 1)
  {
    if(data_len != 19)
    {
      return 1;
    }
  }
  if(type == 2)
  {
    if(data_len != 10)
    {
      return 1;
    }
  }
#warning add_1223
  if(type == 3)
  {
    if(data_len != 49)
    {
      return 1;
    }
  }
  if(addr != data[0])
  {
    return 1;
  }
  parm_len = data[2];
  crc = *(data+ 3 + parm_len + 1 )<<8 | *(data+ 3 + parm_len);
  if(crc != crc16_modbus(data, parm_len+3))
  {
    return 1;
  }
  return 0;
}

static bool IsValidStatus(uint8_t* pStatusBuf, int times, uint8_t curStatus)
{
  int i;
  for (i=times-1; i>0; i--)
  {
    pStatusBuf[i] = pStatusBuf[i-1];
  }
  pStatusBuf[0] = curStatus;
  
  for (i=0; i<times-1; i++)
  {
    if (pStatusBuf[i] != pStatusBuf[i+1])
    {
      return false;
    }
  }
  return true;
}

static uint8_t parse_modbus_pkt(uint8_t* buf, uint8_t data_len, uint8_t type)
{
  if(type == 0)
  {
    memcpy(&(Monitor.meteor), buf+3+8, 22);
    Monitor.meteor.Dm = (buf[3+3] << 8) | buf[2+3];
    Monitor.meteor.Sm = (buf[9+3] << 8) | buf[8+3];
    memcpy(&(Monitor.light), buf+3+34, 4);
    
    if(Monitor.meteor.PM10 == 0 && Monitor.meteor.PM2 == 0)
    {
      memset(&(Monitor.light),0, 4);
    }
  }
  else if(type == 1)
  {
    memcpy(&Monitor.gas, buf+3, 14);
  }
  else if(type == 2)
  {
    Monitor.elec_leak.Water = (uint16_t)BCDtoDec(buf+3,2);
    return SET;
  }
#warning add_1223  
  else if(type == 3)
  {
    memcpy(&(Monitor.meteor), buf+3+8, 16);
    Monitor.meteor.Dm = (buf[3+3] << 8) | buf[2+3];
    Monitor.meteor.Sm = (buf[9+3] << 8) | buf[8+3];
    
    memcpy(&(Monitor.gas), buf+3+24, 14);
    
    Monitor.meteor.NS = (buf[37+3] << 8) | buf[36+3];
    Monitor.meteor.PM2 = (buf[39+3] << 8) | buf[38+3];
    Monitor.meteor.PM10 = (buf[41+3] << 8) | buf[40+3];
  }
  return RESET;
}

static uint8_t get_alarm_result()
{
  SysConf_t config = {0};
  get_config(&config);
  for(uint8_t i=0; i<ALARM_NUM; i++)
  {
    if(config.Alarm[i].alarm_id == 1 && config.Alarm[i].enFlag == SET)
    {
      if(Monitor.elec_leak.Water >= config.Alarm[i].alarm_value)
      {
        Sensor_State[2].value = Monitor.elec_leak.Water;
        return SET;
      }
      else if(Monitor.elec_leak.Water <= config.Alarm[i].recover_value)
      {
        Sensor_State[2].value = Monitor.elec_leak.Water;
        return RECOVER;
      }
    }
  }
  return RESET;
}

static void DA_set_alarm(uint8_t ret, uint8_t type)
{
  if(ret != 0)
  {
    if(ret != Sensor_State[type].eventFlag)
    {
      Sensor_State[type].eventFlag = ret;
      //保存状态
      set_sys_sensor_State(&Sensor_State[type], type);
      if(MsgDeal_Send(get_DA_service_handler(), get_event_service_handler(), MSG_ID_EVENT_ALARM, &Sensor_State[type], sizeof(Sensor_State_t)) != 0)
      {
        loginf("event alarm send to event service fail !\r\n");
      }
    }
  }
}

static void set_fault(uint8_t ret, uint8_t type)
{
  if(ret != Sensor_State[type].errorFlag)
  {
    Sensor_State[type].errorFlag = ret; 
    if(MsgDeal_Send(get_DA_service_handler(), get_event_service_handler(), MSG_ID_EVENT_FAULT, &Sensor_State[type], sizeof(Sensor_State_t)) != 0)
    {
      loginf("event fault send to event service fail !\r\n");
    }
  }
}

/*  
 * return 0  : 收到对应的应答
 * return 1 : 收到应答，但不是本次的应答, 或485校验失败
 */
static int MsgAckHandle(MSG_BUF_t* recvbuf, uint8_t type, uint8_t addr)
{
  int ret = 0x01;
  if(recvbuf->mtype == MSG_ID_DA_DATARCV)
  {
    ret = get_modbus_reply(recvbuf->buf, recvbuf->len, type, addr);
    if(ret != 0)
    {
      //485 add 判断
      return ret;
    }
    else
    {
      int alarm_ret = 0x01;
      uint8_t  type_alarm_flag = RESET;
      //处理收到的数据
      type_alarm_flag = parse_modbus_pkt(recvbuf->buf, recvbuf->len, type);
      if(type_alarm_flag == SET)
      {
        alarm_ret = get_alarm_result();//获取报警状态
        if(IsValidStatus(Sensor_State[type].evt_status_buf, 3, alarm_ret))
        {
          DA_set_alarm(alarm_ret, type);
        }
      }
      return ret;
    }
  }
  return ret;
}

static void wait_modbus_ack(uint8_t type, uint8_t addr)
{
  int ret = 0x01;
  MSG_BUF_t* recv_buf = NULL;
  OS_Q* pQ = get_DA_service_ack_handler();
  
  MsgDeal_Clr(pQ);
  for(int i=0; i<3; i++) /*等待三次，最多3秒 */
  {
    if(MsgDeal_Recv(pQ,&recv_buf, 1000) == 0)
    {
      ret = MsgAckHandle(recv_buf, type, addr);
      MsgDeal_Free(recv_buf);
      if(ret == 0)
      {     
        ret = 0;
        break;
      }
    }
  }
  if(IsValidStatus(Sensor_State[type].err_status_buf, 3, ret))
  {
    set_fault(ret, type);
  }
}

void Meteor_Data(uint8_t type, uint8_t addr)
{
    //memset(&Monitor.meteor, 0, sizeof(Meteor));
    send_modbus_cmd(addr, 0x0000, 19);
    wait_modbus_ack(type, addr);
    loginf("Meteor_Data\r\n");
}

void Gas_Data(uint8_t type, uint8_t addr)
{
    //memset(&Monitor.gas, 0, sizeof(Gas));
    send_modbus_cmd(addr, 0x002A, 7);
    wait_modbus_ack(type, addr);
    loginf("Gas_Data\r\n");
}

void Water_Data(uint8_t type, uint8_t addr)
{
    Monitor.elec_leak.Water = 0;
    send_modbus_cmd(addr, 0x0000, 1);
    wait_modbus_ack(type, addr);
    loginf("Water_Data\r\n");
}

void Meteor_Gas_Data(uint8_t type, uint8_t addr)
{
  //二合一
  send_modbus_cmd(addr, 0x001E, 22);
  wait_modbus_ack(type, addr);
  loginf("Meteor_Gas_Data\r\n");
}

R_Data r_data[DEV_NUM] ={
  {0, Meteor_Data},
  {1, Gas_Data},
  {2, Water_Data},
  {3, Meteor_Gas_Data},
};


//处理收到的485回复数据
//void Sensor_data_process(void)
//{
//  SysConf_t config = {0};
//  get_config(&config);
//  uint8_t addr;
//  //校验错误
////  if(ERROR == get_modbus_reply(DAbuf, data_len, addr))
////  {
////    return;
////  }
//  //数据处理
//  for(uint8_t i=0; i<DEV_NUM; i++)
//  {
//    for(uint8_t j=0;j<DEV_NUM;j++)
//    {
//     if(addr == config.Sensor[i].addr && config.Sensor[i].type == r_data[j].type)
//     {
//       if(config.Sensor[i].enFlag == SET)
//       {
//         r_data[j].fun(SENSOR_PROSESS, addr);
//       }
//     }
//    }
//  }
//}
  


//void alarm(void)
//{
//  //报警
//  for(uint8_t k=0; k<DEV_NUM; k++)
//  {
//    if(Sensor_State[k].errorFlag == RESET || Sensor_State[k].errorFlag == RECOVER_NO_RECIVE)
//    {
//      //三次未回复
//      //if(Sensor_State[k].errorCounter > 2)
//      {
//        Sensor_State[k].errorFlag = SET_NO_RECIVE;
//        if(MsgDeal_Send(get_DA_service_handler(), get_event_service_handler(), MSG_ID_EVENT_FAULT, &Sensor_State[k], sizeof(Sensor_State_t)) != 0)
//        {
//          loginf("error set send to event service fail !\r\n");
//        }
//      }
//    }
//    //故障恢复
//    else if(Sensor_State[k].errorFlag == RECOVER_NO_RECIVE_SET)
//    {
//        Sensor_State[k].errorFlag = RECOVER_NO_RECIVE;
//        if(MsgDeal_Send(get_DA_service_handler(), get_event_service_handler(), MSG_ID_EVENT_FAULT, &Sensor_State[k], sizeof(Sensor_State_t)) != 0)
//        {
//          loginf("error recover send to event service fail !\r\n");
//        }
//    }
//    //事件
//    if(Sensor_State[k].eventFlag == RESET || Sensor_State[k].eventFlag == RECOVER_NO_RECIVE)
//    {
//      //if(Sensor_State[k].eventCounter > 2)
//      {
//        Sensor_State[k].eventFlag = SET_NO_RECIVE;
//        set_sys_sensor_State(&(Sensor_State[k]), k);  
//        if(MsgDeal_Send(get_DA_service_handler(), get_event_service_handler(), MSG_ID_EVENT_ALARM, &Sensor_State[k], sizeof(Sensor_State_t)) != 0)
//        {
//          loginf("event set send to event service fail !\r\n");
//        }
//      }
//    }
//    //事件恢复
//    else if(Sensor_State[k].eventFlag == SET || Sensor_State[k].eventFlag == SET_NO_RECIVE)
//    {
//        Sensor_State[k].eventFlag = RECOVER_NO_RECIVE;
//        set_sys_sensor_State(&(Sensor_State[k]), k);
//        if(MsgDeal_Send(get_DA_service_handler(), get_event_service_handler(), MSG_ID_EVENT_ALARM, &Sensor_State[k], sizeof(Sensor_State_t)) != 0)
//        {
//          loginf("event recover send to event service fail !\r\n");
//        }
//      }
//    }
//  }
//}
static void Sensor_StateInit(void)
{
  //重启初始化时只保存 alarm事件
  SysConf_t config = {0};
  get_config(&config);
  for(uint8_t i=0; i<DEV_NUM; i++)
  {
    if(config.Sensor_State[i].eventFlag != RESET)
    {
      memcpy(&(Sensor_State[i]), (&config.Sensor_State[i]), sizeof(Sensor_State_t));
    }
  }
  Sensor_State[0].type = 0;
  Sensor_State[1].type = 1;
  Sensor_State[2].type = 2;
  Sensor_State[3].type = 3;
}

void LSpeed_485poll(void)
{
#warning modify
  if(get_device_state() != DEVICE_STATE_UPGRADE)
  {
    SysConf_t config = {0};
    get_config(&config);
    for(uint8_t i=0; i<DEV_NUM; i++)
    {
      for(uint8_t j=0;j<DEV_NUM;j++)
      {	
        if(config.Sensor[i].enFlag == SET && config.Sensor[i].type == r_data[j].type)
        {
          r_data[j].fun(config.Sensor[i].type, config.Sensor[i].addr);
        }
      }
    }
  }
  OS_ERR err;
  /* 继续轮询 */
  OSTmrStart (&DA_tmr,&err);
  if(err != OS_ERR_NONE)
    logerr("DA service tmr restart fail !\r\n");
  //logdbg("DA service restart\r\n");
}

static void handle_msg(MSG_BUF_t* msg)
{
  switch(msg->mtype)
  {
  case MSG_ID_DA_DATA_TIMEOUT:     
    if(DA_Init_flag == RESET)
    {
      DA_Init_counter++;
      if(DA_Init_counter >= 13)
      {
        DA_Init_counter = 0;
        DA_Init_flag = SET;
      }
    }
    LSpeed_485poll();
    break;
  default:
    loginf("DA service ID err\r\n");
    break;
  }
}

void DA_tmr_callback(void *p_tmr, void *p_arg)
{
  if(MsgDeal_Send(NULL, get_DA_service_handler(), MSG_ID_DA_DATA_TIMEOUT, NULL, 0) != 0)
  {
    loginf("tm3 send to DA service fail !\r\n");
  }
}

//漏电
void HSpeed_485poll(void)
{
//  SysConf_t config = {0};
//  get_config(&config);
//  for(uint8_t i=0; i<DEV_NUM; i++)
//  {
//    if(config.Sensor[i].enFlag == SET && config.Sensor[i].type == 3)
//    {
//       //Elec_Data(config.Sensor[i].addr);
//    }
//  }
}

void start_DA_service(void)
{
  OSMutexCreate(&DAlock, "DAlock", &DA_err);
  assert(DA_err == OS_ERR_NONE);
  
  OSQCreate (&DA_service_q,"DA_service_q",10,&DA_err);
  assert (DA_err == OS_ERR_NONE);
  
  OSQCreate (&DA_service_ack_q, "DA_service_ack_q",10, &DA_err);
  assert (DA_err == OS_ERR_NONE);
  
  OSTmrCreate((OS_TMR               *)&DA_tmr,	//定时器分辨率100ms 
               (CPU_CHAR             *)"DA_tmr",
               (OS_TICK               )50,		//初始化延迟 50*100ms = 5s
               (OS_TICK               )50,		//周期 50*100ms = 5s
               (OS_OPT                )OS_OPT_TMR_ONE_SHOT,	//有初始化延迟的周期定时器模式
               (OS_TMR_CALLBACK_PTR   )DA_tmr_callback,
               (void                 *)0,
               (OS_ERR               *)&DA_err);
  assert (DA_err == OS_ERR_NONE);
  
  OSTaskCreate((OS_TCB     *)&DA_service_tcb,                               /* Create the startup task                              */
               (CPU_CHAR   *)"DA service",
               (OS_TASK_PTR )DA_service,
               (void       *)0u,
               (OS_PRIO     )APP_CFG_DA_TASK_PRIO,
               (CPU_STK    *)&DA_service_stk[0u],
               (CPU_STK_SIZE)APP_CFG_DA_TASK_STK_SIZE / 10u,
               (CPU_STK_SIZE)APP_CFG_DA_TASK_STK_SIZE,
               (OS_MSG_QTY  )0u,
               (OS_TICK     )0u,
               (void       *)0u,
               (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
               (OS_ERR     *)&DA_err);
  assert (DA_err == OS_ERR_NONE) ;
  
}

void DAInit(void)
{
  DA_uart_init();
  Sensor_StateInit();
  HAL_UART_Receive_DMA(&huart2, DAbuf, DATALEN_MAX);
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
  OSTmrStart(&DA_tmr,&DA_err);//开启定时器
  assert (DA_err == OS_ERR_NONE) ;
}

/* 服务 */
static  void  DA_service(void *p_arg)
{
  MSG_BUF_t* recv_info  = NULL;
  DAInit();
  while (1)
  {
    if(MsgDeal_Recv(get_DA_service_handler(), &recv_info, 5000) == 0)
    {
      handle_msg(recv_info);
      MsgDeal_Free(recv_info);
    }
    set_task_alive_state(DA_SERVICE_TASK);
    //HSpeed_485poll();
  }
}

void USART2_IRQHandler(void)
{
  uint32_t temp;
  CPU_SR_ALLOC();
  CPU_CRITICAL_ENTER();
  OSIntEnter();                                               /* Tell uC/OS-III that we are starting an ISR           */
  CPU_CRITICAL_EXIT();
  
  if((__HAL_UART_GET_FLAG(&huart2,UART_FLAG_IDLE) != RESET))  
  {   
    __HAL_UART_CLEAR_IDLEFLAG(&huart2);  
    HAL_UART_DMAStop(&huart2);
    temp = huart2.hdmarx->Instance->CNDTR;
    data_len = DATALEN_MAX - temp;
    if(data_len > DATALEN_MAX)
    {
      data_len = 0;
    }
  }
  HAL_UART_IRQHandler(&huart2);
  HAL_UART_Receive_DMA(&huart2,DAbuf, DATALEN_MAX);
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
  if(data_len != 0)
  {
    if(MsgDeal_Send(NULL, get_DA_service_ack_handler(), MSG_ID_DA_DATARCV, DAbuf, data_len) != 0)
    {
      loginf("send to DA service fail !\r\n");
    }
  }
  OSIntExit();                                                /* Tell uC/OS-III that we are leaving the ISR */
}

void DMA1_Channel6_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel6_IRQn 0 */

  /* USER CODE END DMA1_Channel6_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart2_rx);
  /* USER CODE BEGIN DMA1_Channel6_IRQn 1 */

  /* USER CODE END DMA1_Channel6_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel7 global interrupt.
  */
void DMA1_Channel7_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel7_IRQn 0 */

  /* USER CODE END DMA1_Channel7_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart2_tx);
  /* USER CODE BEGIN DMA1_Channel7_IRQn 1 */

  /* USER CODE END DMA1_Channel7_IRQn 1 */
}