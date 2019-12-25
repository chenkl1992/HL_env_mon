#include "os.h"
#include "stdarg.h"
#include "printf-stdarg.h"
#include "log.h"
#include  <assert.h>

static OS_MUTEX mutex_log;

//LOG打印级别
volatile unsigned char log_level = LOG_LEVEL_ERR;

OS_MUTEX* log_mutex_id_get()
{
  return &mutex_log;
}

void log_level_set(uint8_t new_level)
{
  log_level = new_level;
}

/*******************************************************************************
*函数名: log_init
*说明: 打印相关初始化
*参数: 无
*返回: 0 - OK
       -1 - ERROR
其他: 无
*******************************************************************************/
//int log_init(void)
//{
//#if DEBUG_PRINTF_EN > 0
//    OS_ERR os_err;
//  
//    OSMutexCreate (&mutex_log, "log mutex", &os_err);
//    if(os_err != OS_ERR_NONE)
//        return -1;
//#endif    
//    return 0;
//}

UART_HandleTypeDef huart4;

void debug_init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_UART4_CLK_ENABLE();
  
  
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    assert(0);
  }
  
  /**UART4 GPIO Configuration    
  PC10     ------> UART4_TX
  PC11     ------> UART4_RX 
  */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  __HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE);
  
  HAL_NVIC_SetPriority(UART4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(UART4_IRQn);
  
  OS_ERR err ;
  OSMutexCreate (&mutex_log,"log mutex",&err);
  assert(err == OS_ERR_NONE);
}


/*******************************************************************************
*函数名: log_printf
*说明: 自定义LOG printf函数
*参数: format - 格式化字符串
*返回: 0 - OK
       -1 - ERROR
其他: 无
*******************************************************************************/
int log_printf(const char *format, ...)
{
#if DEBUG_PRINTF_EN > 0
    va_list args;
    int ret;
    OS_ERR os_err;

    OSMutexPend(&mutex_log,
                2000,
                OS_OPT_PEND_BLOCKING,
                (void *)0,
                &os_err);
    if(os_err != OS_ERR_NONE)
    {
        return -1;
    }
    
    va_start( args, format );
    
    ret = printk_va( 0, format, args );

   OSMutexPost(&mutex_log,
                OS_OPT_POST_NONE,
                &os_err);
    return ret;
#else
    return 0;
#endif
}

