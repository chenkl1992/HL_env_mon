#ifndef __NET_SERVICE_H__
#define __NET_SERVICE_H__

#include  "os.h"
#define DEVICE_STATE_NOT_RDY            0
#define DEVICE_STATE_STAND_BY           1
#define DEVICE_STATE_WORK               2
#define DEVICE_STATE_UPGRADE            3

uint8_t get_device_state(void);
void set_device_state(uint8_t dev_state);
OS_TCB get_net_service_tcb(void);

OS_Q*  get_net_service_handler(void);
void start_net_service(void);
#endif