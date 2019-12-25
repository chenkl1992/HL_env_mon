#ifndef __DA_SERVICE_H__
#define __DA_SERVICE_H__

#include  "os.h"

#define RECOVER                     2
#define SET_NO_RECIVE               3
#define RECOVER_NO_RECIVE           4
#define RECOVER_NO_RECIVE_SET       5

#define DEV_NUM                         4
#define ALARM_NUM                       2

#pragma  pack(1)
typedef struct Meteor{
	uint16_t Dm;    	//风向
	uint16_t Sm;		//风速
	int16_t Ta;		//气温
	uint16_t Ua;		//湿度
	uint16_t Pa;		//气压
	uint16_t Rai;		//雨量
	
	uint16_t Rad;   	//辐射
	uint16_t Uit;		//紫外线
	uint16_t NS;    	//噪声
	uint16_t PM2;		//PM2.5
	uint16_t PM10;	
}Meteor;  //22

typedef struct Gas{
	uint32_t CO;
	uint16_t SO2;
	uint16_t H2S;
	uint16_t NO2;
	uint16_t O3;
	uint16_t NO;
}Gas;//14


typedef struct Elec_Leak{
	uint16_t Water;		//水位
	uint16_t Elec;          //漏电
}Elec_Leak;  //4
#pragma  pack()

__packed typedef struct {
	struct Meteor meteor;
	struct Gas gas;
	struct Elec_Leak elec_leak;
        uint32_t light;
}Monitor_Data;


OS_TCB get_DA_service_tcb(void);
void get_Monitor(Monitor_Data* Monitor_data);
void start_DA_service(void);
void get_sensor_State(Sensor_State_t* State, uint8_t type);
void set_sensor_State(Sensor_State_t* State, uint8_t type);
void set_sys_sensor_State(Sensor_State_t* State, uint8_t type);

extern uint8_t DA_Init_flag;

OS_Q*  get_DA_service_handler(void);
#endif