#ifndef __TCP_SERVICE_H__
#define __TCP_SERVICE_H__

#include  "os.h"

#define LOGDEFAULT                      0
#define LOGIN                           1
#define LOGOUT                          2


OS_Q*  get_tcp_service_handler(void);
void start_tcp_service(void);

OS_TCB get_tcp_service_tcb(void);

void close_tcp_conn(void);

uint8_t get_register_state(void);
void set_register_state(uint8_t reg_state);
int parse_data_stream(void  *buf, uint16_t len, char **pstart, uint16_t* senddata_len, int16_t* errordata_len);
#endif