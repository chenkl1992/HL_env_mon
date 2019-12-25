#ifndef __SDS_TABLE_H__
#define __SDS_TABLE_H__

#include "sFlash.h"

#ifdef __cplusplus 
extern "C" {
#endif
  
#define  NO_UPDAGRADE           0
#define  NEED_UPDAGRADE         1
#define  FINISH_UPDAGRADE       2
#define  SELF_TEST_FAIL         3
#define  RECOVER_SUCCESS        4
#define  RECOVER_FAIL           5  
  
  typedef struct{
    uint8_t type;
    uint8_t errorFlag;
    uint8_t eventFlag;
    uint8_t err_status_buf[3];
    uint8_t evt_status_buf[3];
    uint16_t value;
  }Sensor_State_t;
  //数组顺序表示 4个外设的状态 
  
  typedef struct
  {
    uint8_t enFlag;
    uint8_t type;
    uint8_t addr;
  }Sensor_t;
  
  typedef struct
  {
    uint16_t alarm_value;
    uint16_t recover_value;
    uint8_t enFlag;
    uint8_t alarm_id;
  }Alarm_t;
  
  typedef struct
  {
    uint8_t         upgradeState;
    uint8_t  	dev_addr[6];
    uint32_t 	soft_version;
    uint8_t         hard_version[4];
    Sensor_t	Sensor[4];	
    uint16_t 	report_interval;
    uint8_t		report_mode;
    uint8_t         device_id_setFlag;
    Sensor_State_t  Sensor_State[4];
    Alarm_t         Alarm[2];
  }SysConf_t;

typedef struct  
{
        uint8_t         flag;
	uint8_t 	remoteip[4];
	uint16_t 	remoteport;
	uint8_t 	ip[4];
	uint8_t 	netmask[4];
	uint8_t 	gateway[4];
}lwip_dev_t;  
  
extern SysConf_t sysconfig; 
#ifdef __cplusplus
}
#endif

#endif /* __SDS_PORT_H__ */
