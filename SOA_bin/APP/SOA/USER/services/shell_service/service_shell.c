/*******************************************************************************
* Filename : service_shell.c
* Version  : V1.0
********************************************************************************
* Note(s)  : 
*******************************************************************************/


/*******************************************************************************
*                                 INCLUDE FILES
*******************************************************************************/
#include "stm32f1xx_hal.h"
#include "os.h"
#include "service_shell.h"
#include "app_cfg.h"
#include "shell.h"
#include "com_shell.h"
#include "log.h"
#include "hl_msgdeal.h"
#include "tcp_service.h"
#include "data_service.h"
#include "string.h"
#include "format.h"

/*******************************************************************************
*                                 LOCAL DEFINES
*******************************************************************************/
#define MSG_SHELL_NUM                                           10

#define MSG_SHELL_EXEC_NUM                                      2

/*******************************************************************************
*                             LOCAL GLOBAL VARIABLES
*******************************************************************************/
static OS_TCB tcb_shell;
static CPU_STK stk_shell[OS_STK_SIZE_SERVICE_SHELL];

static OS_Q msg_id_shell_exec;

static OS_MEM mem_id_shell_msg_t;

/*******************************************************************************
*                            LOCAL FUNCTION PROTOTYPES
*******************************************************************************/
static void service_shell(void *param);
static int32_t shell_comm_process(void);

extern void tmr_shell_callback(void *p_tmr, void *p_arg);

int32_t service_shell_init(void* param)
{
    OS_ERR os_err;
    
    OSTaskCreate((OS_TCB*       )&tcb_shell,
                 (CPU_CHAR*     )"task shell",
                 (OS_TASK_PTR   )service_shell,
                 (void*         )param,
                 (OS_PRIO       )OS_PRO_SERVICE_SHELL,
                 (CPU_STK*      )&stk_shell[0],
                 (CPU_STK_SIZE  )OS_STK_SIZE_SERVICE_SHELL / 10,
                 (CPU_STK_SIZE  )OS_STK_SIZE_SERVICE_SHELL,
                 (OS_MSG_QTY    )0,
                 (OS_TICK       )0,
                 (void*         )0,
                 (OS_OPT        )OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR,
                 (OS_ERR*       )&os_err);
    if(os_err != OS_ERR_NONE)
    {
        return -1;
    }
    
    return 0;
}


static void service_shell(void *param)
{
    int32_t ret;
    char *line;
    char *reply;
  
    shell_queue_init();
    shell_printf(COM1SHELL_MSG_BANNER);
    
    while(1)
    {
        if(shell_mode_get() == SHELL_MODE_CMD)
        {
            ret = shell_line_get();
            if(ret <= 0)
                continue;
            
            ret = shell_line_data_get(&line);
            if(ret <= 0)
                continue;
            
            Shell_exec((signed char *)line, 
                       (signed char **)&reply);
            
            if(reply != NULL)
            {
                shell_printf(reply);
            }
        }
        else
        {   
            //关闭LOG
            log_level_set(LOG_LEVEL_CMD);
            
            shell_comm_process();
            
            //开启LOG
            log_level_set(LOG_LEVEL_ALL);
        }
    }
}

OS_MEM *shell_mem_id_get(void)
{
    return &mem_id_shell_msg_t;
}

int32_t shell_exec_msg_create(void)
{
    OS_ERR os_err;
  
    OSQCreate ( (OS_Q*      )&msg_id_shell_exec,
                (CPU_CHAR*  )"msg shell exec",
                (OS_MSG_QTY )MSG_SHELL_EXEC_NUM,
                (OS_ERR*    )&os_err);
    if(OS_ERR_NONE != os_err)
    {
        logerr("shell exec msg create error %d\r\n", os_err);
        return -1;
    }

    return 0;
}

OS_Q* shell_exec_msg_id_get(void)
{
    return &msg_id_shell_exec;
}

int32_t shell_exec_msg_del(void)
{
    OS_ERR os_err;
    
    OSQDel (&msg_id_shell_exec, OS_OPT_DEL_ALWAYS, &os_err);
    if(OS_ERR_NONE != os_err)
    {
        logerr("shell exec msg del error %d\r\n", os_err);
        return -1;
    }
    
    return 0;
}

uint8_t packed_data[256] = {0};
uint16_t serialbuf_len = 0;

void uart_pkt_handler(char* buffer,  uint32_t len)
{
  int16_t errorData_len = 0;
  uint16_t packet_len = 0;
  char* dataptr = buffer;
  uint8_t pack_state = ERROR; 
  //logcmd("receive serial data , data length = %d\r\n" , len);
  if(serialbuf_len + len > 256)
  {
    logcmd("serial_len error\r\n");
    return;
  }
  memcpy(packed_data + serialbuf_len, buffer, len);
  serialbuf_len += len;
  while(serialbuf_len)
  {
    pack_state = parse_data_stream(packed_data, serialbuf_len, &dataptr, &packet_len, &errorData_len);
    if(pack_state == SUCCESS)
    {
      //loginf("pack_state SUCCESS %d\r\n", packet_len);
      /* 获得 数据的地址和长度后 ,将数据传输至 其他服务解析及控制 */   
      if(MsgDeal_Send(NULL, get_data_service_handler(), MSG_ID_SERIAL_PACK, dataptr, packet_len) != 0)
      {
        logcmd("shell pack send to data service fail !\r\n");       
      }
    }
    else
    {
      if(errorData_len <0 )
      {
        serialbuf_len = 0;
        logcmd("Not Find 0x68\r\n");
        break;
      }
      else if(errorData_len == 0)
      {
        break;
      }
      logcmd("error data\r\n");
    }
    serialbuf_len -= errorData_len;
    //拷贝pbuf 剩下的数据
    memcpy(packed_data, packed_data + errorData_len, serialbuf_len);
  }
}

uint8_t buf[256] = {0};
static int32_t shell_comm_process(void)
{
    char c;
    char string[256] = {0};
    uint16_t queue_cnt = 0;
    uint16_t datalen = 0;
    uint8_t modecng_cnt = 0;

    OS_ERR os_err;
    int32_t ret;
    
    while(1)
    {
      OSTimeDly(100, OS_OPT_TIME_DLY, &os_err);   
      while(1)
      {
        ret = queue_pop(shell_uart_queue_get(), &c);
        if(ret != 0 )
        {
          break;
        }
        if(c == '+')
        {
          modecng_cnt ++;
          if(modecng_cnt > 2)
          {
            shell_mode_set(SHELL_MODE_CMD);
            return -1;
          }
        }
        else
        {
          modecng_cnt = 0;
        }
        string[queue_cnt] = c;
        queue_cnt++;
      }
      if(queue_cnt > 0)
      {
        datalen = string2Hexs(string, buf, queue_cnt);
        uart_pkt_handler((char*)buf, datalen);
        memset(buf, 0, 256);
        queue_cnt = 0;
      }
    }
}
