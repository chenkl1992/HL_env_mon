#ifndef __DATA_SERVICE_H__
#define __DATA_SERVICE_H__

#include  "os.h"
#include  "hl_packet.h"

#define HARD_VERSION   			0x00000000
#define DEV_TYPE   			0x0201
//重启原因
#define CAUSE_SYS_RESET                        0
#define CAUSE_NO_RESPONSE                      1
#define CAUSE_RECONNECT                        2

//注销原因
#define OUT_CAUSE_SYS_RESET                    0x10
#define OUT_CAUSE_UPGRADE                      0x11
#define OUT_CAUSE_SYSERROR                     0x12

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

typedef struct{
  uint8_t RevisionLen;
  uint32_t SoftwareVersion;
  char Revision[24];
}Revision_t;

extern Revision_t Revision;
extern SysConf_t sysconfig;

void get_config(SysConf_t* config);
void get_addr(char* data);
void set_log_cause(uint8_t cause);
void set_config_upgradeFlag(uint8_t upgrade);
void set_config_addr(uint8_t* addr);
uint16_t get_seqID(void);
void factory_resrt(void);

void sysconfig_edit(void);
void sysconfig_save(void);

OS_TCB get_data_service_tcb(void);
OS_Q*  get_data_service_handler(void);
uint32_t get_upgrade_software_version(void);
uint8_t get_heartbeat_counter(void);
void set_heartbeat_zero(void);
void set_heartbeat_counter(void);
void software_versionInit(void);
void set_config_report(uint8_t mode , uint16_t interval);
void clear_devId(void);

void start_data_service(void);
void send_login_info(uint8_t* cause, uint16_t* seq_id);
void send_logout_info(uint8_t* cause);
void send_event_data(Sensor_State_t* Sensor_State, uint16_t value, uint16_t* seqID);
void send_error_data(Sensor_State_t* Sensor_State, uint16_t* seqID);
uint8_t set_sensor(byte* data, uint8_t datalen);
uint8_t set_alarm(byte* data, uint8_t datalen);

void send_heart_beat(void);
#endif