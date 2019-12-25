#include "stm32f1xx_hal.h"
#include "os.h"
#include "cmd_api_msg.h"

/*******************************************************************************
*函数名: msg_create_queue
*说明: 消息队列创建
*参数: id - 消息队列ID(消息队列控制块地址)
       p_name - 消息队列名称
       size - 消息队列大小
*返回: 0 - 创建成功
       -1 - 创建出错
其他: 无
*******************************************************************************/
int32_t msg_create_queue(void *id, const char *p_name, uint32_t q_size)
{
    OS_ERR os_err;
  
    OSQCreate ((OS_Q     *)id,
               (CPU_CHAR *)p_name,
               (OS_MSG_QTY)q_size,
               (OS_ERR   *)&os_err);
    if(OS_ERR_NONE != os_err)
    {
        return -1;
    }
      
    return 0;
}

/*******************************************************************************
*函数名: msg_send
*说明: 发送消息
*参数: id - 消息队列ID(消息队列控制块地址)
       msg - 消息数据存储地址
       size - 消息数据大小
*返回: 0 - 发送成功
       -1 - 发送出错
其他: 无
*******************************************************************************/
int32_t msg_send(void *id, void *msg, uint32_t size)
{
    OS_ERR os_err;
  
    OSQPost ((OS_Q      *)id,
            (void       *)msg,
            (OS_MSG_SIZE )size,
            (OS_OPT      )OS_OPT_POST_FIFO,
            (OS_ERR     *)&os_err);
    if(OS_ERR_NONE != os_err)
    {
        return -1;
    }
      
    return 0;
}

/*******************************************************************************
*函数名: msg_recv
*说明: 等待接收消息
*参数: id - 消息队列ID(消息队列控制块地址)
       msg - 接收到的消息数据存储地址
       size - 接收到的消息数据大小
       timeout - 接收等到超时时间
*返回: 0 - 接收成功
       -1 - 接收出错
其他: 无
*******************************************************************************/
int32_t msg_recv(void *id, void **msg, uint32_t *size, uint32_t timeout)
{
    OS_ERR os_err;
    OS_MSG_SIZE s;
    
    *msg = OSQPend ((OS_Q        *)id,
                    (OS_TICK      )timeout,
                    (OS_OPT       )OS_OPT_PEND_BLOCKING,
                    (OS_MSG_SIZE *)&s,
                    (CPU_TS      *)NULL,
                    (OS_ERR      *)&os_err);
    //类型转换
    *size = s;
    if(OS_ERR_NONE != os_err)
    {
        *msg = NULL;
        return -1;
    }
    
    return 0;
}
