#include "sFlash.h"
#include "sFlash_cfg.h"
#include "stm32f1xx_hal.h"
#include <assert.h>

void _sFlash_SPI_Init(void)
{
	SFLASH_CLK_ENABLE();
	
	GPIO_InitTypeDef  gpio = {0};
	
	gpio.Pin        = sFlash_PinInfo.MOSI.pin;
	gpio.Speed      = GPIO_SPEED_FREQ_HIGH;
	gpio.Pull       = GPIO_PULLUP;
	gpio.Mode       = GPIO_MODE_AF_PP;
	HAL_GPIO_Init(sFlash_PinInfo.MOSI.port, &gpio);
		
	gpio.Pin        = sFlash_PinInfo.CLK.pin;
	gpio.Speed      = GPIO_SPEED_FREQ_HIGH;
	gpio.Pull       = GPIO_PULLUP;
	gpio.Mode       = GPIO_MODE_AF_PP;
	HAL_GPIO_Init(sFlash_PinInfo.CLK.port, &gpio);
		
	gpio.Pin        = sFlash_PinInfo.MISO.pin;
	gpio.Speed      = GPIO_SPEED_FREQ_HIGH;
	gpio.Pull       = GPIO_PULLUP;
	gpio.Mode       = GPIO_MODE_AF_PP;
	HAL_GPIO_Init(sFlash_PinInfo.MISO.port, &gpio);
	
	HAL_SPI_Init(&sFlash_SPI_Handle);
	
}

void _sFlash_SPI_Tx(const uint8_t* buf, uint16_t len)
{
	HAL_StatusTypeDef status = HAL_SPI_Transmit(&sFlash_SPI_Handle, (uint8_t *)buf, len, 100 * len);
	assert(status == HAL_OK);
}

void _sFlash_SPI_Rx(uint8_t* buf, uint16_t len)
{
	HAL_StatusTypeDef status = HAL_SPI_Receive(&sFlash_SPI_Handle, buf, len, 100 * len);
	assert(status == HAL_OK);
}

void _sFlash_SPI_TxRx(const uint8_t* tx_buf, uint8_t* rx_buf, uint16_t len)
{
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&sFlash_SPI_Handle, (uint8_t *)tx_buf, rx_buf, len, 100 * len);
	assert(status == HAL_OK);
}

// 初始化后,CS应拉高
void _sFlash_CS_Init(void)
{
	GPIO_InitTypeDef  gpio = {0};
		
	gpio.Pin        = sFlash_PinInfo.CS.pin;
	gpio.Speed      = GPIO_SPEED_FREQ_HIGH;
	gpio.Pull       = GPIO_PULLUP;
	gpio.Mode       = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(sFlash_PinInfo.CS.port, &gpio);
}

void _sFlash_CS_Low(void)
{
	HAL_GPIO_WritePin(sFlash_PinInfo.CS.port, sFlash_PinInfo.CS.pin, GPIO_PIN_RESET);	
}

void _sFlash_CS_High(void)
{
	HAL_GPIO_WritePin(sFlash_PinInfo.CS.port, sFlash_PinInfo.CS.pin, GPIO_PIN_SET);		
}

void _sFlash_WP_Init(void)
{
#ifdef SFLASH_WP
	GPIO_InitTypeDef  gpio = {0};
	
	gpio.Pin        = sFlash_PinInfo.WP.pin;
	gpio.Speed      = GPIO_SPEED_FREQ_HIGH;
	gpio.Pull       = GPIO_PULLUP;
	gpio.Mode       = GPIO_MODE_OUTPUT_PP;
	HAL_GPIO_Init(sFlash_PinInfo.WP.port, &gpio);
#endif /* SFLASH_WP */
}

void _sFlash_WP_Low(void)
{
#ifdef SFLASH_WP
	HAL_GPIO_WritePin(sFlash_PinInfo.WP.port, sFlash_PinInfo.WP.pin, GPIO_PIN_RESET);	
#endif /* SFLASH_WP */
}

void _sFlash_WP_High(void)
{
#ifdef SFLASH_WP
	HAL_GPIO_WritePin(sFlash_PinInfo.WP.port, sFlash_PinInfo.WP.pin, GPIO_PIN_SET);		
#endif /* SFLASH_WP */
}

uint32_t _sFlash_GetTickMs(void)
{
	return HAL_GetTick();
}

void _sFlash_Sleep(uint32_t ms)
{
	HAL_Delay(ms);
}

#if SFLASH_MULTI_TASK > 0

static OS_MUTEX _flash_mutex;

/// <summary>
/// [接口函数]Flash互斥锁初始化
/// </summary>
/// <returns>
/// SFLASH_ERR_NONE      : 成功
/// SFLASH_ERR_LOCK_INIT : 互斥锁初始化失败
/// </returns>
uint8_t _sFlash_Lock_Init(void)
{
	OS_ERR err;
	OSMutexCreate (&_flash_mutex, "_flash_mutex", &err);
	if (err == OS_ERR_NONE)    return SFLASH_ERR_NONE;
	return SFLASH_ERR_LOCK_INIT;
}

/// <summary>
/// [接口函数]Flash进入临界区
/// </summary>
/// <returns>
/// SFLASH_ERR_NONE         : 成功
/// SFLASH_ERR_LOCK_TIMEOUT : 进入临界区超时
/// SFLASH_ERR_LOCK         : 进入临界区失败
/// </returns>
uint8_t _sFlash_Lock(void)
{
	OS_ERR err;
	OSMutexPend(&_flash_mutex, SFLASH_PEND_MS, OS_OPT_PEND_BLOCKING, NULL, &err);
	if (err == OS_ERR_NONE)    return SFLASH_ERR_NONE;
	if (err == OS_ERR_TIMEOUT) return SFLASH_ERR_LOCK_TIMEOUT;
	return SFLASH_ERR_LOCK;
}

/// <summary>
/// [接口函数]Flash退出临界区
/// </summary>
/// <returns>
/// SFLASH_ERR_NONE         : 成功
/// SFLASH_ERR_LOCK         : 退出临界区失败
/// </returns>
uint8_t _sFlash_Unlock(void)
{
	OS_ERR err;
	OSMutexPost(&_flash_mutex, OS_OPT_POST_NONE, &err);
	if (err == OS_ERR_NONE)    return SFLASH_ERR_NONE;
	return SFLASH_ERR_LOCK;
}

#endif /* SFLASH_MULTI_TASK > 0 */
