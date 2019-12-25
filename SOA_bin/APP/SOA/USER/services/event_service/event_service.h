#ifndef __EVENT_SERVICE_H__
#define __EVENT_SERVICE_H__

#include  "os.h"

typedef struct
{
  uint8_t eventStartFlag;
  uint8_t errorStartFlag;
  Sensor_State_t Sensor_State;
}EventConf_t;

OS_Q*  get_event_service_ack_handler(void);
OS_TCB get_event_service_tcb(void);
void start_event_service(void);
OS_Q*  get_event_service_handler(void);
#endif