#ifndef __SFLASH_CFG_H__
#define __SFLASH_CFG_H__

#include "sFlash_port.h"
#include "stm32f1xx_hal.h"

#if (USE_SPI_CRC != 0U)
#error "please set USE_SPI_CRC to 0"
#endif /* USE_SPI_CRC */

static SPI_HandleTypeDef sFlash_SPI_Handle = 
{
	.Instance               = SPI3, 
	.Init.Mode              = SPI_MODE_MASTER, 
	.Init.Direction         = SPI_DIRECTION_2LINES,
	.Init.DataSize          = SPI_DATASIZE_8BIT,
	.Init.CLKPolarity       = SPI_POLARITY_HIGH,
	.Init.CLKPhase          = SPI_PHASE_2EDGE,
	.Init.NSS               = SPI_NSS_SOFT,
	.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2,
	.Init.FirstBit          = SPI_FIRSTBIT_MSB,
	.Init.TIMode            = SPI_TIMODE_DISABLE,
	.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE,
	.Init.CRCPolynomial     = 7,
};

//#define SFLASH_WP
#undef  SFLASH_WP

static const sFlash_PinInfo_t sFlash_PinInfo = 
{
	.CS     = { .port = GPIOB, .pin = GPIO_PIN_6  },
	.CLK    = { .port = GPIOB, .pin = GPIO_PIN_3  },
	.MOSI   = { .port = GPIOB, .pin = GPIO_PIN_5  },
	.MISO   = { .port = GPIOB, .pin = GPIO_PIN_4  },
#ifdef SFLASH_WP
	.WP     = { .port =      , .pin =             },
#endif /* SFLASH_WP */
};


//#define SFLASH_CLK_ENABLE()  do { \
//                                        __HAL_RCC_AFIO_CLK_ENABLE(); \
//                                        __HAL_AFIO_REMAP_SWJ_NOJTAG(); \
//                                        __HAL_RCC_SPI2_CLK_ENABLE(); \
//                                        __HAL_RCC_GPIOB_CLK_ENABLE(); \
//                                      } while(0U)
#define SFLASH_CLK_ENABLE()  do { \
                                       __HAL_RCC_AFIO_CLK_ENABLE(); \
                                        __HAL_AFIO_REMAP_SWJ_NOJTAG(); \
                                        __HAL_RCC_SPI3_CLK_ENABLE(); \
                                        __HAL_RCC_GPIOB_CLK_ENABLE(); \
                                      } while(0U)										  

// 需要多任务操作 Flash ?
// !!! 当配置为 0 时,禁止多个任务使用此模块 !!!
#define SFLASH_MULTI_TASK         0          /* 0: Disable  1: Enable */   


// 获取操作权限时,等待的最大毫秒数(仅多任务操作时有效) 
#define SFLASH_PEND_MS            50000      



// --- 选择芯片 ---
// 支持的芯片列表:
//   SST25VF080B
//   SST25VF016
//   W25Q16DV
//   W25Q32FV
//   W25Q64FV
//   W25Q128FV
#define SFLASH_CHIP               W25Q16DV     

										  
#endif /* __SFLASH_CFG_H__ */