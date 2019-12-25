#include  "stm32f1xx_hal.h"
#include  "app_cfg.h"
#include  "clk.h"
#include  <assert.h>
#include  <stdbool.h>
#include  "hl_msgdeal.h"
#include  "hl_packet.h"
#include  "net_service.h"
#include  "tcp_service.h"
#include  "udp_service.h"
#include  "data_service.h"
#include  "update_service.h"
#include  "register_service.h"
#include  "heartbeat_service.h"
#include  "event_service.h"
#include  "DA_service.h"
#include  "supervision_service.h"
#include  "lwip.h"
#include  "sds.h"
#include  "log.h"
#include  "format.h"

static  OS_Q  		data_service_q;
static  void  		data_service (void  *p_arg);
static  OS_TCB   	data_service_tcb;
static  CPU_STK  	data_service_stk[APP_CFG_DATA_TASK_STK_SIZE];

#define OPCODE_SET      1
#define OPCODE_DEL      2
#define OPCODE_DELALL   0

volatile static uint16_t seq_ID = 0;
uint8_t ip_changeFlag = RESET;

uint8_t serial_cmdFlag = RESET;

OS_MUTEX Datalock;

static void MsgHandle(MSG_BUF_t *msg);
void software_versionInit(void);
void factory_resrt(void);

OS_Q*  get_data_service_handler(void)
{
  return &data_service_q;
}

OS_TCB get_data_service_tcb(void)
{
  return data_service_tcb;
}

void start_data_service(void)
{
    OS_ERR  os_err;
    
    OSQCreate (&data_service_q,"data_service_q",10,&os_err);
    assert (os_err == OS_ERR_NONE) ;
    
    OSMutexCreate(&Datalock, "Datalock", &os_err);
    assert(os_err == OS_ERR_NONE);
    
    OSTaskCreate((OS_TCB     *)&data_service_tcb,                               /* Create the startup task                              */
                 (CPU_CHAR   *)"data service",
                 (OS_TASK_PTR )data_service,
                 (void       *)0u,
                 (OS_PRIO     )APP_CFG_DATA_TASK_PRIO,
                 (CPU_STK    *)&data_service_stk[0u],
                 (CPU_STK_SIZE)APP_CFG_DATA_TASK_STK_SIZE / 10u,
                 (CPU_STK_SIZE)APP_CFG_DATA_TASK_STK_SIZE,
                 (OS_MSG_QTY  )0u,
                 (OS_TICK     )0u,
                 (void       *)0u,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&os_err);
    assert (os_err == OS_ERR_NONE) ;
}

static void data_service(void *p_arg)
{
  (void)p_arg;
  MSG_BUF_t* recv_info  = NULL;
  //software_versionInit();
#warning 111  
  //factory_resrt();
  while(DEF_TRUE)
  {
    if(MsgDeal_Recv(get_data_service_handler(),&recv_info, 5000) == 0)
    {
      MsgHandle(recv_info);
      MsgDeal_Free(recv_info);
    }
    set_task_alive_state(DATA_SERVICE_TASK);
    //OSTimeDly (10,OS_OPT_TIME_DLY,&err);
  }
}
/***********************************************************以下为数据处理部分************************************************************************/
/*主命令*/
#define SET_COMMAND				0x01
#define GET_COMMAND				0x02
#define CTL_COMMAND				0x03
#define ACK_COMMAND				0x80
#define UPDATE_COMMAND			        0xF1
#define REGIST_COMMAND                          0xA1
#define REPORT_COMMAND                          0xA2
#define EVENT_COMMAND                           0xA3
/**/

/*子命令*/
#define DEVICE_STATE                            0x0C

#define PID_OFFSET				25
#define APP_ADDR				0x08008800ul            //bootloader 32K + 2K
#define APP_INFO_AT				APP_ADDR - 64

#define LOCAL_IP_CHANGE                         0x01
#define REMOTE_IP_CHANGE                        0x02

#define SET_SUCCESS                             0
#define SET_ERROR                               1

static volatile bool need_reboot = false;
static volatile bool need_upgrade = false;
static uint32_t upgrade_software_version = 0;
static uint8_t heartbeat_counter = 0;

uint8_t dev_state = DEVICE_STATE_NOT_RDY;
SysConf_t sysconfig = {0};
char stringData[256] = {0};

typedef void (*cbFun)(PARAM_ITEM_S*);

typedef struct
{
	cbFun 	set_callback;  		//设置和控制类回调
	cbFun 	get_callback;		//查询类回调
}FunItem_t;


Revision_t Revision = {0};

void sysconfig_edit(void)
{
  SDS_Edit(&sysconfig);
}

void sysconfig_save(void)
{
  SDS_Save(&sysconfig);
}

void FlashReadStr( uint32_t flash_add, uint16_t len, uint16_t* data )
{
 uint16_t byteN = 0;
 uint16_t byteC = 0;
 uint16_t testData = 0;
 uint16_t i = 0;
 while( i<len )
 {
  testData = *(uint16_t*)(flash_add+byteN);
  data[byteC] = testData;
  byteN += 2;
	byteC++;
	i++;
 }
}

void get_config(SysConf_t* config)
{
  OS_ERR  os_err;
  OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  *config = sysconfig;
  OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);	
}

uint32_t get_upgrade_software_version(void)
{
  return upgrade_software_version;
}

uint8_t get_heartbeat_counter(void)
{
  return heartbeat_counter;
}

void set_heartbeat_counter(void)
{
  heartbeat_counter++;
}

void set_heartbeat_zero(void)
{
  heartbeat_counter = 0;
}

//初始化软件版本号等
void software_versionInit(void)
{
  Img_head_t head = {0};
  FlashReadStr(APP_INFO_AT, sizeof(Img_head_t)/2, (uint16_t*)&head);
  
  sysconfig.Sensor[0].type = 0;
  sysconfig.Sensor[1].type = 1;
  sysconfig.Sensor[2].type = 2;
  sysconfig.Sensor[3].type = 3;
  //if(head.softVer == 0xFFFFFFFF)
  loginf("soft ver: %2x\r\n", head.softVersion);
  loginf("upgradeState: %d\r\n", sysconfig.upgradeState);
//  if(1)
//  {
//    Revision.SoftwareVersion = config.soft_version;
//    Revision.RevisionLen = 2;
//    Revision.Revision[0] = 'r';
//    Revision.Revision[1] = 0;
//  }
//  else
  {
    Revision.SoftwareVersion = head.softVersion;
    sysconfig.soft_version = head.softVersion;
    char* str1 = NULL;
    char* str2 = NULL;
    char* str3 = NULL;
    uint8_t namelen;
    str1 = strstr((char*)head.fileName, "-");
    if(str1 == NULL)
    {
      Revision.RevisionLen = 1;
      Revision.Revision[0] = 0;
    }
    else
    {
      namelen = strlen(str1);
      str2 = strstr(str1, "r");
      str3 = strstr(str1, "t");
      if(str2 != NULL)
      {
        Revision.RevisionLen = 1;
        Revision.Revision[0] = 'r';
        Revision.Revision[1] = 0;
      }
      else if(str3 != NULL)
      {
        Revision.RevisionLen = namelen - 1;
        memcpy(Revision.Revision, str3, Revision.RevisionLen);
        Revision.Revision[Revision.RevisionLen] = 0;
      }
    }
  }
}

uint16_t get_seqID(void)
{
  //OS_ERR  os_err;
  //OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  //assert(os_err == OS_ERR_NONE);
  seq_ID++;
  //OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  //assert(os_err == OS_ERR_NONE);
  return seq_ID;
}

void get_addr(char* data)
{
  OS_ERR  os_err;
  OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  data[0] = (DEV_TYPE >> 8) & 0xFF;
  data[1] = (DEV_TYPE >> 0) & 0xFF;
  data[2] = sysconfig.dev_addr[0];
  data[3] = sysconfig.dev_addr[1];
  data[4] = sysconfig.dev_addr[2];
  data[5] = sysconfig.dev_addr[3];
  data[6] = sysconfig.dev_addr[4];
  data[7] = sysconfig.dev_addr[5];
  OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
}

void set_config_addr(uint8_t* addr)
{
  OS_ERR  os_err;
  OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  sysconfig_edit();
  sysconfig.device_id_setFlag = SET;
  sysconfig.dev_addr[0] = addr[0];
  sysconfig.dev_addr[1] = addr[1];
  sysconfig.dev_addr[2] = addr[2];
  sysconfig.dev_addr[3] = addr[3];
  sysconfig.dev_addr[4] = addr[4];
  sysconfig.dev_addr[5] = addr[5];
  sysconfig_save();
  OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
  SDS_Poll();
}

void set_config_upgradeFlag(uint8_t upgrade)
{
  OS_ERR  os_err;
  OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  sysconfig_edit();
  sysconfig.upgradeState = upgrade;
  sysconfig_save();
  OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
  SDS_Poll();
}

uint8_t set_sensor(byte* data, uint8_t datalen)
{
  uint8_t opcode;
  if((datalen-1) > 2*4)
  {
    return SET_ERROR;
  }
  if((datalen-1)%2 != 0)
  {
    return SET_ERROR;
  }
  opcode = data[0];
  if(opcode > OPCODE_DEL)
  {
    return SET_ERROR;
  }
  OS_ERR  os_err;
  OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  sysconfig_edit();
  if(opcode == OPCODE_DELALL)
  {
    memset(sysconfig.Sensor, 0, sizeof(Sensor_t)*4);
    memset(sysconfig.Sensor_State, 0, sizeof(Sensor_State_t)*4);
    set_sensor_State(&(sysconfig.Sensor_State[0]), 0);
    set_sensor_State(&(sysconfig.Sensor_State[1]), 1);
    set_sensor_State(&(sysconfig.Sensor_State[2]), 2);
    set_sensor_State(&(sysconfig.Sensor_State[3]), 3);
    sysconfig.Sensor[0].type = 0;
    sysconfig.Sensor[1].type = 1;
    sysconfig.Sensor[2].type = 2;
    sysconfig.Sensor[3].type = 3;
  }
  else
  {
    for(uint8_t j=1; j<datalen; j+=2)
    {
      for(uint8_t i=0; i<DEV_NUM; i++)
      {
        if(sysconfig.Sensor[i].type == data[j])
        {
          sysconfig.Sensor[i].type = data[j];
          sysconfig.Sensor[i].addr = data[j+1];
          if(opcode == OPCODE_SET)
          {
            sysconfig.Sensor[i].enFlag = SET;
          }
          else if(opcode == OPCODE_DEL)
          {
            sysconfig.Sensor[i].enFlag = RESET;
            Sensor_State_t tmpSensor_State = {0};
            set_sensor_State(&tmpSensor_State, sysconfig.Sensor[i].type);
          }
        }
      }
    }
  }
  sysconfig_save();
  OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
  return SET_SUCCESS;
}

uint8_t set_alarm(byte* data, uint8_t datalen)
{
  uint16_t alarm_value;
  uint16_t recover_value;
  uint8_t alarm_id;
  uint8_t enFlag;
  if(datalen > 2*6)
  {
    return SET_ERROR;
  }
  if(datalen%6 != 0)
  {
    return SET_ERROR;
  }
  OS_ERR  os_err;
  OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  sysconfig_edit();
  for(uint8_t i=0, j=0; j<datalen; i++)
  {
    alarm_id = data[j];
    if(alarm_id > 1)
    {
      return SET_ERROR;
    }
    alarm_value = data[j+2]<<8 | data[j+1];
    recover_value = data[j+4]<<8 | data[j+3];
    if(recover_value > alarm_value)
    {
      return SET_ERROR;
    }
    enFlag = data[j+5];
    if(enFlag > 1)
    {
      return SET_ERROR;
    }
    sysconfig.Alarm[i].alarm_id = alarm_id;
    sysconfig.Alarm[i].alarm_value = alarm_value;
    sysconfig.Alarm[i].recover_value = recover_value;
    sysconfig.Alarm[i].enFlag = enFlag;
    j = j + 6;
  }
  sysconfig_save();
  OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
  return SET_SUCCESS;
}

void factory_resrt(void)
{
  OS_ERR  os_err;
  OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  sysconfig_edit();
  memset(sysconfig.Alarm, 0, sizeof(Alarm_t)*2);
  memset(sysconfig.Sensor, 0, sizeof(Sensor_t)*4);
  memset(sysconfig.Sensor_State, 0, sizeof(Sensor_State_t)*4);
  sysconfig.report_mode = 0;
  sysconfig.report_interval = 30;
  sysconfig_save();
  
//  uint8_t remote_data[4] = {192, 168, 1, 211};
//  uint16_t port = 6000;
//  uint8_t ip[4] = {192, 168, 1, 174};
//  uint8_t netmask[4] = {255, 255, 255, 0};
//  uint8_t gateway[4] = {192, 168, 1, 1};

#warning 222  
//  uint8_t remote_data[4] = {192, 168, 10, 10};
//  uint16_t port = 6000;
//  uint8_t ip[4] = {192, 168, 10, 53};
//  uint8_t netmask[4] = {255, 255, 255, 0};
//  uint8_t gateway[4] = {192, 168, 1, 1};
//  
//  set_lwip_remote_para(remote_data, port);
//  set_lwip_local_para(ip, netmask, gateway);
  
  OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
  SDS_Poll();
}

void clear_devId(void)
{
  OS_ERR  os_err;
  OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  sysconfig_edit();
  memset(sysconfig.dev_addr, 0, 6);
  sysconfig.device_id_setFlag = RESET;
  sysconfig_save();
  OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
  SDS_Poll();
}

void set_config_report(uint8_t mode , uint16_t interval)
{
  OS_ERR  os_err;
  OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  sysconfig_edit();
  sysconfig.report_mode = mode;
  sysconfig.report_interval = interval;	
  sysconfig_save();
  OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
  SDS_Poll();
}
#define LOGIN_SUCCESS  0
#define LOGIN_ALREADY  1
/*回调函数区*/

void get_deviceinfo_cb(PARAM_ITEM_S* para)
{
	uint8_t ack_dlen = 0;
	para->data[ack_dlen++] = 0;
	
	SysConf_t config = {0};
	get_config(&config);
	
	para->data[ack_dlen++] = config.hard_version[0];
	para->data[ack_dlen++] = config.hard_version[1];
	para->data[ack_dlen++] = config.hard_version[2];
	para->data[ack_dlen++] = config.hard_version[3];
        
        /* 软件版本 */
	para->data[ack_dlen++] = config.soft_version & 0x000000FF;
	para->data[ack_dlen++] = (config.soft_version >> 8 ) & 0x000000FF;
	para->data[ack_dlen++] = (config.soft_version >> 16 ) & 0x000000FF;
	para->data[ack_dlen++] = (config.soft_version >> 24 ) & 0x000000FF;
        memcpy(para->data+ack_dlen, Revision.Revision, Revision.RevisionLen);
        ack_dlen = ack_dlen + Revision.RevisionLen;

	para->dlen = ack_dlen;
}

void set_id_cb(PARAM_ITEM_S* para)
{ 
  uint8_t ack_dlen = 0;
  if(sysconfig.device_id_setFlag == RESET && serial_cmdFlag == SET)
  {
    set_config_addr(&para->data[0]);
    para->data[ack_dlen++] = SET_SUCCESS;
  }
  else
  {
    para->data[ack_dlen++] = SET_ERROR;
  }
  
  para->dlen = ack_dlen;
}

void set_report_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;
  uint8_t mode;
  uint16_t interval;
  mode = para->data[0];
  interval = para->data[2] << 8 | para->data[1];
  set_config_report(mode, interval);
  para->data[ack_dlen++] = 0;
  para->dlen = ack_dlen;
  if(MsgDeal_Send(NULL, get_heartbeat_service_handler(), MSG_ID_HEARTBEAT_DURATION_CHANGE, NULL, 0) != 0)
  {
    logerr("data service send to heartbeat service fail !\r\n");
  }
  if(MsgDeal_Send(NULL, get_tcp_service_handler(), MSG_ID_HEARTBEAT_DURATION_CHANGE, NULL, 0) != 0)
  {
    logerr("data service send to tcp service fail !\r\n");
  }
}

void get_report_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;
  para->data[ack_dlen++] = 0;
  
  SysConf_t config = {0};
  get_config(&config);
  para->data[ack_dlen++] = config.report_mode;
  para->data[ack_dlen++] = config.report_interval & 0x00FF;
  para->data[ack_dlen++] = (config.report_interval >> 8) & 0x00FF;
  para->dlen = ack_dlen;
}

void reboot_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;
  need_reboot = true;
  para->data[ack_dlen++] = 0;
  para->dlen = ack_dlen;	
}

void factory_reset_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;
  factory_resrt();
  para->data[ack_dlen++] = 0;
  para->dlen = ack_dlen;
  //ip_changeFlag = LOCAL_IP_CHANGE;
}

void set_hard_version(uint8_t* version)
{
  OS_ERR  os_err;
  OSMutexPend(&Datalock, 0, OS_OPT_PEND_BLOCKING, NULL, &os_err);
  assert(os_err == OS_ERR_NONE);
  sysconfig_edit();
  sysconfig.hard_version[0] = version[0];
  sysconfig.hard_version[1] = version[1];
  sysconfig.hard_version[2] = version[2];
  sysconfig.hard_version[3] = version[3];
  sysconfig_save();
  OSMutexPost(&Datalock, OS_OPT_POST_NONE, &os_err);
  assert(os_err == OS_ERR_NONE);
  SDS_Poll();
}

void set_hard_version_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;  
  if(para->dlen == 4 && serial_cmdFlag == SET)
  {
    set_hard_version(&para->data[0]);
    para->data[ack_dlen++] = SET_SUCCESS;
  }
  else
  {
    para->data[ack_dlen++] = SET_ERROR;
  }
  para->dlen = ack_dlen;
}

void upgrade_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;
  uint8_t ret = 0xFF;
  SysConf_t config = {0};
  get_config(&config);
  
  //高版本升级
  memcpy(&upgrade_software_version, para->data+1, sizeof(uint32_t));
  if(para->data[0] == 0)
  {
    if(upgrade_software_version > config.soft_version)
    {
      ret = 0;
    }
    else
    {
      logerr("version error\r\n");
    }
  }
  else if(para->data[0] == 1)
  {
    ret = 0;
  }
  /* 在flash上清理出存储空间 */
  if(ret == 0)
  {
      need_upgrade = true;
  }
  /* 数据包回复 */		
  para->data[ack_dlen++] = ret;
  para->dlen = ack_dlen;
}
void set_ip_cb(PARAM_ITEM_S* para)
{
	uint8_t ack_dlen = 0;
	set_lwip_local_para(&para->data[0] , &para->data[4] , &para->data[8]);
	para->data[ack_dlen++] = 0;
	para->dlen = ack_dlen;
        ip_changeFlag = LOCAL_IP_CHANGE;
}

void get_ip_cb(PARAM_ITEM_S* para)
{
	uint8_t ack_dlen = 0;
	
	lwip_dev_t ip_dev = {0};
	get_lwip_dev(&ip_dev);
	
	para->data[ack_dlen++] = 0;
	
	para->data[ack_dlen++] = ip_dev.ip[0];
	para->data[ack_dlen++] = ip_dev.ip[1];
	para->data[ack_dlen++] = ip_dev.ip[2];
	para->data[ack_dlen++] = ip_dev.ip[3];

	para->data[ack_dlen++] = ip_dev.netmask[0];
	para->data[ack_dlen++] = ip_dev.netmask[1];
	para->data[ack_dlen++] = ip_dev.netmask[2];
	para->data[ack_dlen++] = ip_dev.netmask[3];

	para->data[ack_dlen++] = ip_dev.gateway[0];
	para->data[ack_dlen++] = ip_dev.gateway[1];
	para->data[ack_dlen++] = ip_dev.gateway[2];
	para->data[ack_dlen++] = ip_dev.gateway[3];
	
	para->dlen = ack_dlen;
}

void set_server_cb(PARAM_ITEM_S* para)
{
	uint8_t ack_dlen = 0;
	set_lwip_remote_para(&para->data[0] , para->data[4] << 8 | para->data[5]);
	para->data[ack_dlen++] = 0;
	para->dlen = ack_dlen;
        ip_changeFlag = REMOTE_IP_CHANGE;
}

void get_server_cb(PARAM_ITEM_S* para)
{
	uint8_t ack_dlen = 0;
	
	lwip_dev_t ip_dev = {0};
	get_lwip_dev(&ip_dev);
	
	para->data[ack_dlen++] = 0;
	
	para->data[ack_dlen++] = ip_dev.remoteip[0];
	para->data[ack_dlen++] = ip_dev.remoteip[1];
	para->data[ack_dlen++] = ip_dev.remoteip[2];
	para->data[ack_dlen++] = ip_dev.remoteip[3];

	para->data[ack_dlen++] = (ip_dev.remoteport >> 8) & 0xFF;
	para->data[ack_dlen++] = (ip_dev.remoteport >> 0) & 0xFF;
	
	para->dlen = ack_dlen;
}

void search_dev_cb(PARAM_ITEM_S* para)
{
	uint8_t ack_dlen = 0;
	
	lwip_dev_t ip_dev = {0};	
	get_lwip_dev(&ip_dev);
	
	para->data[ack_dlen++] = 0;
	
	para->data[ack_dlen++] = ip_dev.remoteip[0];
	para->data[ack_dlen++] = ip_dev.remoteip[1];
	para->data[ack_dlen++] = ip_dev.remoteip[2];
	para->data[ack_dlen++] = ip_dev.remoteip[3];

	para->data[ack_dlen++] = ip_dev.netmask[0];
	para->data[ack_dlen++] = ip_dev.netmask[1];
	para->data[ack_dlen++] = ip_dev.netmask[2];
	para->data[ack_dlen++] = ip_dev.netmask[3];

	para->data[ack_dlen++] = ip_dev.gateway[0];
	para->data[ack_dlen++] = ip_dev.gateway[1];
	para->data[ack_dlen++] = ip_dev.gateway[2];
	para->data[ack_dlen++] = ip_dev.gateway[3];
	
	para->data[ack_dlen++] = ip_dev.ip[0];
	para->data[ack_dlen++] = ip_dev.ip[1];
	para->data[ack_dlen++] = ip_dev.ip[2];
	para->data[ack_dlen++] = ip_dev.ip[3];
	
	para->data[ack_dlen++] = (ip_dev.remoteport >> 8) & 0xFF;
	para->data[ack_dlen++] = (ip_dev.remoteport >> 0) & 0xFF;
	para->dlen = ack_dlen;
}

void set_sensor_address_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;
  uint8_t state = SET_SUCCESS;
  state = set_sensor(para->data, para->dlen);
  if(state == SET_SUCCESS)
  {
    SDS_Poll();
    if(MsgDeal_Send(NULL, get_event_service_handler(), MSG_ID_EVENT_STOP, NULL, 0) != 0)
    {
      logerr("data service send to event service fail !\r\n");
    }
  }
  para->data[ack_dlen++] = state;
  para->dlen = ack_dlen;
}

void get_sensor_address_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;

  para->data[ack_dlen++] = 0;
  
  for(uint8_t i=0; i<DEV_NUM; i++)
  {
    if(sysconfig.Sensor[i].enFlag == SET)
    {
      para->data[ack_dlen++] = sysconfig.Sensor[i].type;
      para->data[ack_dlen++] = sysconfig.Sensor[i].addr;
    }
  }
  para->dlen = ack_dlen;
}

void set_alarm_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;
  uint8_t state = SET_SUCCESS;
  state = set_alarm(para->data, para->dlen);
  if(state == SET_SUCCESS)
  {
    SDS_Poll();
  }
  para->data[ack_dlen++] = state;
  para->dlen = ack_dlen;
}

void get_alarms_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;

  para->data[ack_dlen++] = 0;
  
  para->data[ack_dlen++] = sysconfig.Alarm[0].alarm_id;
  para->data[ack_dlen++] = (sysconfig.Alarm[0].alarm_value >> 8) & 0xFF;
  para->data[ack_dlen++] = (sysconfig.Alarm[0].alarm_value >> 0) & 0xFF;
  para->data[ack_dlen++] = (sysconfig.Alarm[0].recover_value >> 8) & 0xFF;
  para->data[ack_dlen++] = (sysconfig.Alarm[0].recover_value >> 0) & 0xFF;
  para->data[ack_dlen++] = sysconfig.Alarm[0].enFlag;
  
  para->data[ack_dlen++] = sysconfig.Alarm[1].alarm_id;
  para->data[ack_dlen++] = (sysconfig.Alarm[1].alarm_value >> 8) & 0xFF;
  para->data[ack_dlen++] = (sysconfig.Alarm[1].alarm_value >> 0) & 0xFF;
  para->data[ack_dlen++] = (sysconfig.Alarm[1].recover_value >> 8) & 0xFF;
  para->data[ack_dlen++] = (sysconfig.Alarm[1].recover_value >> 0) & 0xFF;
  para->data[ack_dlen++] = sysconfig.Alarm[1].enFlag;
  
  para->dlen = ack_dlen;
}

void get_device_state_cb(PARAM_ITEM_S* para)
{
  uint8_t ack_dlen = 0;
  Monitor_Data Monitor = {0};
  get_Monitor(&Monitor);
  memcpy(para->data, &Monitor, sizeof(Monitor_Data));
  ack_dlen = sizeof(Monitor_Data);
  para->dlen = ack_dlen;
}

void heartbeat_ack_cb(PARAM_ITEM_S* para, uint16_t seq)
{
  if(para->data[0] == 0)
  {
    heartbeat_counter = 0;
  }
}

void regist_ack_cb(PARAM_ITEM_S* para, uint16_t seq)
{
  if(para->data[0] == LOGIN_SUCCESS || para->data[0] == LOGIN_ALREADY)
  {
    if(MsgDeal_Send(NULL, get_register_service_ack_handler(), MSG_ID_LOG_IN_ACK, &seq, 2) != 0)
    {
      logerr("data service send to register service fail !\r\n");
    }
  }
}

void event_ack_cb(PARAM_ITEM_S* para, uint16_t seq)
{
  if(para->data[0] == 0)
  {
    uint8_t evevt_data[3];
    memcpy(evevt_data, &seq, 2);
    evevt_data[2] = para->pid;
    if(MsgDeal_Send(NULL, get_event_service_ack_handler(), MSG_ID_EVENT_ACK, evevt_data, 3) != 0)
    {
      logerr("data service send to event service fail !\r\n");
    }
  }
}

//参数回调表
const static FunItem_t cbFunTab[] = 
{
	{NULL				, NULL				},	//0x00
	{NULL				, NULL			        },	//0x01
	{NULL				, get_deviceinfo_cb	        },	//0x02
	{set_id_cb			, NULL			        },	//0x03
	{set_report_cb		        , get_report_cb		        },	//0x04
	{NULL				, NULL				},	//0x05
	{reboot_cb			, NULL				},	//0x06
	{NULL				, NULL				},	//0x07
	{upgrade_cb			, NULL				},	//0x08
	{set_ip_cb			, get_ip_cb			},	//0x09
	{set_server_cb		        , get_server_cb		        },	//0x0A
	{search_dev_cb		        , NULL			        },	//0x0B
	{NULL				, get_device_state_cb		},	//0x0C
	{NULL				, NULL				},	//0x0D
	{NULL				, NULL				},	//0x0E
	{NULL				, NULL				},	//0x0F
	{NULL				, NULL				},	//0x10
        {factory_reset_cb		, NULL				},	//0x11
	{NULL				, NULL				},	//0x12
	{set_alarm_cb			, get_alarms_cb			},	//0x13
        {NULL				, NULL				},	//0x14
        {NULL				, NULL				},	//0x15
	{set_hard_version_cb	        , NULL		                },	//0x16
        
	{set_sensor_address_cb		, get_sensor_address_cb		},	//0x30
	{NULL		, NULL				},			//0x31
	{NULL		, NULL				},			//0x32
};



void set_handle(PARAM_ITEM_S* param, int num)
{
	cbFun pFunction = NULL;
	for(int i = 0 ; i < num ; i++)
	{
		if(param[i].pid < 0x30)
			pFunction = cbFunTab[param[i].pid].set_callback;
		else
			pFunction = cbFunTab[param[i].pid - PID_OFFSET].set_callback;
		if(pFunction != NULL)
			pFunction(&param[i]);
	}
}

void get_handle(PARAM_ITEM_S* param, int num)
{
  cbFun pFunction = NULL;
  for(int i = 0 ; i < num ; i++)
  {
    if(param[i].pid < 0x30)
      pFunction = cbFunTab[param[i].pid].get_callback;
    else
      pFunction = cbFunTab[param[i].pid - PID_OFFSET].get_callback;
    if(pFunction != NULL)
      pFunction(&param[i]);
  }
}

void do_req_handler(byte main_cmd, PARAM_ITEM_S* param, int num)
{
  switch(main_cmd)
  {
  case SET_COMMAND:
    set_handle(param, num);
    break;
  case GET_COMMAND:
    get_handle(param, num);
    break;
  case CTL_COMMAND:
    set_handle(param , 1);
    break;
  case UPDATE_COMMAND:
    set_handle(param , 1);
    break;
  default:break;
  //           err_handler(param);		
  }
}

void do_ack_handler(byte main_cmd, PARAM_ITEM_S* param, int num, uint16_t seq_num)
{
  switch(main_cmd)
  {
  case REGIST_COMMAND: 
    regist_ack_cb(param, seq_num);
    break;
  case EVENT_COMMAND:
    //事件回复都由一个函数处理
    event_ack_cb(param, seq_num);
    break;
  case REPORT_COMMAND:
    set_heartbeat_zero();
    break;
  default:
    //err_handler(param);
    break;
  }
}

void send_heart_beat(void)
{
  PKT_HEAD_S head = {0};
  PARAM_ITEM_S param = {0};
  Monitor_Data Monitor = {0};
  
  byte req_buf[64] = {0};
  int req_len = 0;
  
  head.head_flg = 0x68;
  head.dev_addr[0] = (DEV_TYPE >> 8) & 0xFF;
  head.dev_addr[1] = (DEV_TYPE >> 0) & 0xFF;
  Mem_Copy(&head.dev_addr[2],sysconfig.dev_addr , 6);	
  head.main_cmd = REPORT_COMMAND;
  head.dlen = sizeof(Monitor_Data)+1;
  head.seq_no = get_seqID();
  
  param.pid = DEVICE_STATE;
  param.dlen = sizeof(Monitor_Data);

  get_Monitor(&Monitor);
  memcpy(param.data, &Monitor, param.dlen);
  
  req_len = pack_req_packet(req_buf, &head, &param, 1);
  
  if(MsgDeal_Send(NULL, get_tcp_service_handler(), MSG_ID_TCP_PACK, req_buf, req_len) != 0)
  {
    logerr("data service send to tcp service heartbeat in pack fail !\r\n");
  }
}

void send_login_info(uint8_t* cause, uint16_t* seq_id )
{
  PKT_HEAD_S head = {0};
  PARAM_ITEM_S param = {0};
  
  byte req_buf[64] = {0};
  int req_len = 0;
  
  head.head_flg = 0x68;
  head.dev_addr[0] = (DEV_TYPE >> 8) & 0xFF;
  head.dev_addr[1] = (DEV_TYPE >> 0) & 0xFF;
  Mem_Copy(&head.dev_addr[2],sysconfig.dev_addr , 6);	
  head.main_cmd = REGIST_COMMAND;
  head.dlen = 0x01;
  head.seq_no = get_seqID();
  *seq_id = head.seq_no;
  param.pid = 0x01;
  //param.dlen = 0;
  param.dlen = 5 + Revision.RevisionLen + 1;
  param.data[0] = *cause;
  Mem_Copy(param.data+1, &(Revision.SoftwareVersion), 4);
  Mem_Copy(param.data+5, Revision.Revision, Revision.RevisionLen);
  
  req_len = pack_req_packet(req_buf, &head, &param, 1);
  
  if(MsgDeal_Send(NULL, get_tcp_service_handler(), MSG_ID_TCP_PACK, req_buf, req_len) != 0)
  {
    logerr("data service send to tcp service login pack fail !\r\n");
  }
}

void send_logout_info(uint8_t* cause)
{
  PKT_HEAD_S head = {0};
  PARAM_ITEM_S param = {0};
  
  byte req_buf[64] = {0};
  int req_len = 0;
  
  head.head_flg = 0x68;
  head.dev_addr[0] = (DEV_TYPE >> 8) & 0xFF;
  head.dev_addr[1] = (DEV_TYPE >> 0) & 0xFF;
  Mem_Copy(&head.dev_addr[2],sysconfig.dev_addr , 6);	
  head.main_cmd = REGIST_COMMAND;
  head.dlen = 0x01;
  head.seq_no = get_seqID();
  
  param.pid = 0x00;
  param.dlen = 1;
  param.data[0] = *cause;
  
  req_len = pack_req_packet(req_buf, &head, &param, 1);
  if(MsgDeal_Send(NULL, get_tcp_service_handler(), MSG_ID_TCP_PACK, req_buf, req_len) != 0)
  {
    logerr("data service send to tcp service logout pack fail !\r\n");
  }
}

void send_event_data(Sensor_State_t* Sensor_State, uint16_t value, uint16_t* seqID)
{
  PKT_HEAD_S head = {0};
  PARAM_ITEM_S param = {0};
  
  byte req_buf[64] = {0};
  int req_len = 0;
  
  head.head_flg = 0x68;
  head.dev_addr[0] = (DEV_TYPE >> 8) & 0xFF;
  head.dev_addr[1] = (DEV_TYPE >> 0) & 0xFF;
  Mem_Copy(&head.dev_addr[2],sysconfig.dev_addr , 6);	
  head.main_cmd = EVENT_COMMAND;
  head.dlen = 0x04;
  head.seq_no = get_seqID();
  *seqID = head.seq_no;
  
  if(Sensor_State->eventFlag == SET)
  {
    param.pid = 0x0D;
  }
  else if(Sensor_State->eventFlag == RECOVER)
  {
    param.pid = 0x0E;
  }
  param.dlen = 3;
  param.data[0] = Sensor_State->type;
  memcpy(param.data+1, &value, sizeof(uint16_t));
  
  req_len = pack_req_packet(req_buf, &head, &param, 1);
  if(MsgDeal_Send(NULL, get_tcp_service_handler(), MSG_ID_TCP_PACK, req_buf, req_len) != 0)
  {
    logerr("event service send to tcp service event pack fail !\r\n");
  }
}

void send_error_data(Sensor_State_t* Sensor_State, uint16_t* seqID)
{
  PKT_HEAD_S head = {0};
  PARAM_ITEM_S param = {0};
  
  byte req_buf[64] = {0};
  int req_len = 0;
  
  head.head_flg = 0x68;
  head.dev_addr[0] = (DEV_TYPE >> 8) & 0xFF;
  head.dev_addr[1] = (DEV_TYPE >> 0) & 0xFF;
  Mem_Copy(&head.dev_addr[2],sysconfig.dev_addr , 6);	
  head.main_cmd = EVENT_COMMAND;
  head.dlen = 0x04;
  head.seq_no = get_seqID();
  *seqID = head.seq_no;
  
  if(Sensor_State->errorFlag == SET)
  {
    param.pid = 0x0F;
  }
  else
  {
    param.pid = 0x10;
  }
  param.dlen = 2;
  param.data[0] = Sensor_State->type+2;
  param.data[1] = 0x01;
  
  req_len = pack_req_packet(req_buf, &head, &param, 1);
  if(MsgDeal_Send(NULL, get_tcp_service_handler(), MSG_ID_TCP_PACK, req_buf, req_len) != 0)
  {
    logerr("event service send to tcp service event pack fail !\r\n");
  }
}

byte get_main_cmd(byte* buf)
{
  byte req_mainId;
  req_mainId = *(buf + sizeof(PKT_HEAD_S));
  return req_mainId;
}

static void MsgHandle(MSG_BUF_t *msg)
{
	byte* buffer = msg->buf;
	uint16_t buflen = msg->len;
        byte main_cmd;
	
	PKT_HEAD_S head = {0};
	PARAM_ITEM_S param[7] = {0};
	int num = 0;
        
        serial_cmdFlag = RESET;
        if(msg->mtype == MSG_ID_SERIAL_PACK)
        {
          serial_cmdFlag = SET;
        }
	//if(msg->mtype == MSG_ID_TCP_PACK)
	{
          /* 检查数据包合法性 */
          uint16_t type = buffer[1] << 8 | buffer[2];
          if(type != DEV_TYPE)
          {
            if(msg->mtype == MSG_ID_TCP_PACK)
            {
              return;
            }
          }
          if(Mem_Cmp(&buffer[3],sysconfig.dev_addr , 6) != DEF_YES)
          {
            if(msg->mtype == MSG_ID_TCP_PACK)
            {
              return;
            }
          }
	}
	if(is_packet(buffer, buflen) != 0)
		return ;
	num = parse_packet(&head, param, buffer, buflen);
	if(num <= 0)
          return ;
	if(head.main_cmd == 0x80)
	{
          if(msg->mtype == MSG_ID_TCP_PACK)
          {
            if(memcmp(&head.dev_addr[2], sysconfig.dev_addr, 6) != 0)
            {
              if(msg->mtype == MSG_ID_TCP_PACK)
              {
                logerr("device id error\r\n");
                return;
              }
            }
            main_cmd = get_main_cmd(buffer);
            do_ack_handler(main_cmd, param, num, head.seq_no);
          }
	}
	else
	{
          uint8_t respondFlag = RESET;
          if(get_device_state() == DEVICE_STATE_WORK)
          {
            respondFlag = SET;
          }
          if(msg->mtype == MSG_ID_SERIAL_PACK)
          {
            respondFlag = SET;
          }
          if(msg->mtype == MSG_ID_UDP_PACK)
          {
            respondFlag = SET;
          }
          if(respondFlag == SET)
          {
            byte ack_buf[128] = {0};
            int ack_len = 0;
            if(memcmp(&head.dev_addr[2], sysconfig.dev_addr, 6) != 0)
            {
              if(msg->mtype == MSG_ID_TCP_PACK)
              {
                logerr("device id error\r\n");
                return;
              }
            }
            do_req_handler(head.main_cmd, param, num);
            head.dev_addr[0] = (DEV_TYPE >> 8) & 0xFF;
            head.dev_addr[1] = (DEV_TYPE >> 0) & 0xFF;
            Mem_Copy(&head.dev_addr[2],sysconfig.dev_addr , 6);
            ack_len = pack_ack_packet(ack_buf, &head, param, num);
            if(msg->mtype == MSG_ID_TCP_PACK)
            {
              if(MsgDeal_Send(get_data_service_handler(), get_tcp_service_handler(), MSG_ID_TCP_PACK, ack_buf, ack_len) != 0)
              {
                logerr("data service send to tcp service ack fail !\r\n");
              }
            }
            else if(msg->mtype == MSG_ID_UDP_PACK)
            {
              if(MsgDeal_Send(get_data_service_handler(), get_udp_service_handler(), MSG_ID_UDP_PACK, ack_buf, ack_len) != 0)
              {
                logerr("data service send to udp service ack fail !\r\n");
              }			
            }
            else if(msg->mtype == MSG_ID_SERIAL_PACK)
            {
              hex2String(ack_buf, ack_len, stringData);
              logcmd("%s", stringData);
              memset(stringData, 0, 256);
            }
            if(ip_changeFlag > RESET )
            {
              if(ip_changeFlag == LOCAL_IP_CHANGE)
              {
                SDS_Poll();
                if(MsgDeal_Send(get_data_service_handler(), get_net_service_handler(), MSG_ID_NET_IP_CHANGE, NULL, 0) != 0)
                {
                  logerr("data service send to tcp service ip change fail !\r\n");
                }
              }
              else if(ip_changeFlag == REMOTE_IP_CHANGE)
              {
                SDS_Poll();
                if(MsgDeal_Send(get_data_service_handler(), get_net_service_handler(), MSG_ID_NET_REMOTE_IP_CHANGE, NULL, 0) != 0)
                {
                  logerr("data service send to tcp service remote ip change fail !\r\n");
                }
              }
              ip_changeFlag = RESET;
            }
            if(need_reboot)
            {
              uint8_t log_cause;
              log_cause = OUT_CAUSE_SYS_RESET;
              if(MsgDeal_Send(get_data_service_handler(), get_register_service_handler(), MSG_ID_LOG_OUT, &log_cause, 1) != 0)
              {
                logerr("tcp pack send to register service fail !\r\n");
              }
            }
            if(need_upgrade)
            {
              need_upgrade = false;
              int ret = 0xFF;
              ret = sFlash_Erase(0, 0x0080000);
              if(ret == 0)
              {
                set_device_state(DEVICE_STATE_UPGRADE);
                if(MsgDeal_Send(get_data_service_handler(), get_update_service_handler(), MSG_ID_UPGRADE_IND, NULL, 0) != 0)
                {
                  logerr(" send to update service fail !\r\n");
                }
              }
            }
          }
        }
}
