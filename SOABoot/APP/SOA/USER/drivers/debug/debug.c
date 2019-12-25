#include "debug.h"
#include <string.h>

OS_MUTEX  	debug_mutex;
OS_ERR		mutex_err;

static UART_HandleTypeDef huart4;


static uint8_t uart4_buf[256] = {0};
static uint16_t uart4_index = 0;

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
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  __HAL_UART_ENABLE_IT(&huart4, UART_IT_RXNE);
  
  HAL_NVIC_SetPriority(UART4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(UART4_IRQn);
  
  OS_ERR err ;
  OSMutexCreate (&debug_mutex,"debug mutex",&err);
  assert(err == OS_ERR_NONE);
}

//void UART4_IRQHandler(void)
//{
//  HAL_UART_IRQHandler(&huart4);
//  
//  uint32_t tmp_flag = 0, tmp_it_source = 0;
//  tmp_flag = __HAL_UART_GET_FLAG(&huart4, UART_FLAG_RXNE);
//  tmp_it_source = __HAL_UART_GET_IT_SOURCE(&huart4, UART_IT_RXNE);
//  if((tmp_flag != RESET) && (tmp_it_source != RESET))
//  {
//    uart4_buf[uart4_index++] = (uint8_t)(huart4.Instance->DR & (uint8_t)0x00FF);
//    if(uart4_index >= sizeof(uart4_buf)) //缓冲区溢出,把之前收到的数据全部抛弃，重新获取
//      uart4_index = 0;	
//  }
//}

//unsigned short debug_rx(char* buf ,unsigned short len)
//{
//	uint16_t real_len = len > uart4_index ? uart4_index : len ;
//	if(real_len == 0)
//		return 0;
//	memcpy(buf,uart4_buf,real_len);
//	memcpy(uart4_buf,uart4_buf + real_len , uart4_index - real_len);
//	memset(uart4_buf + uart4_index - real_len,0,real_len);
//	uart4_index -= real_len;
//	return real_len;
//}
