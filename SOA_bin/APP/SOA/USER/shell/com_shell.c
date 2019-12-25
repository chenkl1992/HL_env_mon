#include "stm32f1xx_hal.h"
#include "printf-stdarg.h"
#include "os.h"
#include "stdarg.h"
#include "string.h"
#include "com_shell.h"
#include "shell.h"
#include "log.h"
#include "hl_msgdeal.h"
#include "service_shell.h"
#include "queue.h"

static queue_t shell_uart_queue = {0};

static struct {
    char data[128];
    uint32_t count;
}shell_uart = {0};

static shell_mode_t shell_mode = SHELL_MODE_CMD;

extern int32_t shell_msg_send(void *dest, void *src, char c);

/*******************************************************************************
*函数名: shell_line_get
*说明:   获取并回显命令
*参数:  无
*返回: 0 - OK
       -1 - ERROR
其他: 无
*******************************************************************************/
int32_t shell_line_get( void )
{
    int32_t ret;
    char c;
    char esc[8];
    uint8_t esc_index = 0;
    OS_ERR os_err;
    uint8_t plus_cnt = 0;

    shell_printf(COM1SHELL_MSG_PROMPT);
    
    memset(&shell_uart, 0, sizeof(shell_uart));
    while(1)
    {
        OSTimeDly(100, OS_OPT_TIME_DLY, &os_err);
        ret = queue_pop(&shell_uart_queue, &c);
        if(ret != 0)
        {
            continue;
        }
        if(c == '+')
        {
            plus_cnt++;
            if(plus_cnt > 2)
            {
                shell_mode_set(SHELL_MODE_COMM);
                return -1;
            }
        }
        else
        {
            plus_cnt = 0;
        }

        switch (c)
        {
        case LF:
           shell_printf("%c", CR);
           shell_printf("%c", c);
           shell_uart.data[shell_uart.count] = '\0'; 
           return(shell_uart.count);
         case CR:
           shell_printf("%c", c);
           shell_printf("%c", LF);
           shell_uart.data[shell_uart.count] = '\0';
           return(shell_uart.count);
         case ABORT_CHAR:
           memset(&shell_uart, 0, sizeof(shell_uart));
           shell_printf(CRLF);
           shell_printf(COM1SHELL_MSG_PROMPT );
           break;

        case BKSPACE_CHAR:
           if(shell_uart.count > 0)
           {
              shell_printf("\b \b" );
              shell_uart.count--;
           }
           break;
           
        case ESC:
          memset(esc, 0, sizeof(esc));
          esc_index = 0;
          
          if(esc_index < sizeof(esc))
            esc[esc_index ++] = c;
          break;
          
        case MI_B:
          if(esc[esc_index - 1] == ESC)
          {
            if(esc_index < sizeof(esc))
                esc[esc_index ++] = c;
          }
          else
          {
            shell_printf("%c", c);
            if(shell_uart.count < sizeof(shell_uart.data) - 1)
                shell_uart.data[shell_uart.count++] = c;
          }
          break;

        default:
           if((esc[esc_index - 2] == ESC) && (esc[esc_index - 1] == MI_B))
           {
                if(esc_index < sizeof(esc))
                    esc[esc_index ++] = c;
                esc_index = 0;
           }
           else
           {    
                shell_printf("%c", c);
                if(shell_uart.count < sizeof(shell_uart.data) - 1)
                    shell_uart.data[shell_uart.count++] = c;
           }
           break;
        }
    }    
}

int32_t shell_printf(char *format, ...)
{
    int32_t ret;
    va_list args;
    OS_ERR os_err;
    
    extern OS_MUTEX *log_mutex_id_get(void);
    
    OSMutexPend(log_mutex_id_get(),
                2000,
                OS_OPT_PEND_BLOCKING,
                (void *)0,
                &os_err);
    if(os_err != OS_ERR_NONE)
    {
        return -1;
    }
    
    va_start(args, format);
    
    ret = printk_va(0, format, args);
    
    va_end(args);

    OSMutexPost(log_mutex_id_get(),
                OS_OPT_POST_NONE,
                &os_err);
    
    return ret;
}

/*******************************************************************************
*函数名: shell_line_data_get
*说明:   获取命令行数据
*参数:  data - 获取到的命令行字符串
*返回: 命令行长度
其他: 无
*******************************************************************************/
int32_t shell_line_data_get(char **data)
{
    *data = shell_uart.data;
    
    return shell_uart.count;
}

/*******************************************************************************
*函数名: isr_uart_shell
*说明:   shell 命令上数据接收中断
*参数:  无
*返回: 无
其他: 无
*******************************************************************************/
void UART4_IRQHandler(void)
{
    UART_HandleTypeDef *uart_shell;
    uint32_t ir;
    uint8_t data;
   
    uart_shell = &huart4;
    
    ir = uart_shell->Instance->SR;
    data = uart_shell->Instance->DR;
    if(ir & USART_SR_RXNE)
    {
        queue_push(&shell_uart_queue, data);
    }
}

void shell_mode_set(shell_mode_t mode)
{
    CPU_SR_ALLOC();
  
    CPU_CRITICAL_ENTER();
    shell_mode = mode;
    CPU_CRITICAL_EXIT();
}

shell_mode_t shell_mode_get(void)
{
    CPU_SR_ALLOC();
    shell_mode_t mode;
  
    CPU_CRITICAL_ENTER();
    mode = shell_mode;
    CPU_CRITICAL_EXIT();
    
    return mode;
}

void shell_queue_init(void)
{
    queue_init(&shell_uart_queue);
}

queue_t *shell_uart_queue_get(void)
{
    return &shell_uart_queue;
}
