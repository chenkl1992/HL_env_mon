#ifndef __SERVICE_SHELL_H
#define __SERVICE_SHELL_H

int32_t service_shell_init(void* param);

int32_t shell_exec_msg_create(void);
OS_Q* shell_exec_msg_id_get(void);
int32_t shell_exec_msg_del(void);

#endif

