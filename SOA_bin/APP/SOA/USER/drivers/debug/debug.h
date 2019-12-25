#ifndef __DEBUG_H
#define __DEBUG_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "os.h"

extern OS_MUTEX  	debug_mutex;
extern OS_ERR		mutex_err;

//#define debug(fmt, args...)     \
//          printf("Debug: " fmt "(file: %s, func: %s, line: %d)\n", ##args, __FILE__, __func__, __LINE__);


//#define debug(fmt, ...)     \
//          printf("Debug: " fmt "(file: %s, func: %s, line: %d)\n", ##__VA_ARGS__, __FILE__, __func__, __LINE__);

//#define debug(fmt, ...)     \
//          printf("Debug: " fmt "(func: %s, line: %d)\n", ##__VA_ARGS__, __func__, __LINE__);


#define debug(fmt, ...)   printf(" %s : %d: " fmt "" , __FILE__,__LINE__, ##__VA_ARGS__)

#define printf_s(fmt, ...)  \
		OSMutexPend (&debug_mutex,0,OS_OPT_PEND_BLOCKING,NULL,&mutex_err);\
		printf("" fmt "" , ##__VA_ARGS__);\
		OSMutexPost (&debug_mutex,OS_OPT_POST_NONE,&mutex_err);

extern void debug_init(void);
extern void debug_tx(char* buf ,unsigned short len);
extern unsigned short debug_rx(char* buf ,unsigned short len);
#endif