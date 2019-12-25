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
	uint16_t Dm;    	//����
	uint16_t Sm;		//����
	int16_t Ta;		//����
	uint16_t Ua;		//ʪ��
	uint16_t Pa;		//��ѹ
	uint16_t Rai;		//����
	
	uint16_t Rad;   	//����
	uint16_t Uit;		//������
	uint16_t NS;    	//����
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
	uint16_t Water;		//ˮλ
	uint16_t Elec;          //©��
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