#ifndef COM_SHELL_H
#define COM_SHELL_H

#include "queue.h"

#define COM1SHELL_MSG_BANNER   "\x0C\r\n---------- Hoolink Shell ----------\r\n"
#define COM1SHELL_MSG_PROMPT   "$>"

typedef enum {
    SHELL_MODE_CMD = 0,
    SHELL_MODE_COMM,
}shell_mode_t;

int shell_line_get( void );

int32_t shell_line_data_get(char **data);

int32_t shell_printf(char *format, ...);

void shell_queue_init(void);

void shell_mode_set(shell_mode_t mode);
shell_mode_t shell_mode_get(void);

queue_t *shell_uart_queue_get(void);

#endif
