#ifndef __UDP_SERVICE_H__
#define __UDP_SERVICE_H__

#include  "os.h"

OS_Q*  get_udp_service_handler(void);
void start_udp_service(void);
OS_TCB get_udp_service_tcb(void);

void udp_mode_set(void);
void udp_mode_reset(void);

#endif