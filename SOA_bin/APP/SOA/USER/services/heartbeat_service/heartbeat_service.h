#ifndef __HEARTBEAT_SERVICE_H__
#define __HEARTBEAT_SERVICE_H__

#include  "os.h"

OS_TCB get_heartbeat_service_tcb(void);

void start_heartbeat_service(void);
OS_Q*  get_heartbeat_service_handler(void);
#endif