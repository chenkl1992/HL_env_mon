#ifndef __SUPERVISION_SERVICE_H__
#define __SUPERVISION_SERVICE_H__

#include  "os.h"

/* 共八个服务 
net_service  	
udp_service
tcp_service
register_service
heartbeat_service
data_service
DA_service
event_service
*/

typedef enum
{
  NET_SERVICE_TASK = 0,
  UDP_SERVICE_TASK,
  TCP_SERVICE_TASK,
  REGISTER_SERVICE_TASK,
  HB_SERVICE_TASK,
  DATA_SERVICE_TASK,
  DA_SERVICE_TASK,
  EVENT_SERVICE_TASK,
}TASK_LIST;


typedef struct
{
  uint8_t not_alive_counter;
  uint8_t before;
  uint8_t current;
}task_state_t;

#define SERVICE_NUM   8

void start_supervision_service(void);
void set_task_alive_state(uint8_t serviceNum);
#endif