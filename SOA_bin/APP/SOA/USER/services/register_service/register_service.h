#ifndef __REGISTER_SERVICE_H__
#define __REGISTER_SERVICE_H__

#include  "os.h"

void start_register_service(void);
OS_Q*  get_register_service_handler(void);
OS_Q*  get_register_service_ack_handler(void);
OS_TCB get_regist_service_tcb(void);
uint8_t get_logout_flag(void);
#endif